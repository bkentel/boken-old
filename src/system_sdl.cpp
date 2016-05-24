#include "system.hpp"           // for read_only_pointer_t, etc
#include "render.hpp"
#include "math.hpp"             // for ceil_as
#include "config.hpp"

#include <bkassert/assert.hpp>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <functional>           // for function
#include <memory>               // for unique_ptr
#include <stdexcept>            // for runtime_error
#include <tuple>                // for tie, tuple
#include <utility>              // for move, pair, swap

#include <cstdint>              // for uint16_t, uint8_t, uint32_t

namespace boken {

namespace {

struct sdl_deleter_window {
    void operator()(SDL_Window* const p) noexcept {
        SDL_DestroyWindow(p);
    }
};

struct sdl_deleter_renderer {
    void operator()(SDL_Renderer* const p) noexcept {
        SDL_DestroyRenderer(p);
    }
};

struct sdl_deleter_surface {
    void operator()(SDL_Surface* const p) noexcept {
        SDL_FreeSurface(p);
    }
};

struct sdl_deleter_texture {
    void operator()(SDL_Texture* const p) noexcept {
        SDL_DestroyTexture(p);
    }
};

//! Exception type to wrap SDL error codes
struct sdl_error : std::runtime_error {
    using runtime_error::runtime_error;
};

//! RAII wrapper around the SDL system
class sdl {
public:
    static constexpr auto systems =
        SDL_INIT_EVENTS | SDL_INIT_VIDEO;

    sdl(sdl const&) = delete;
    sdl& operator=(sdl const&) = delete;

    sdl() {
        if (SDL_WasInit(0) & systems) {
            throw sdl_error {"SDL already initialized"};
        }

        if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
            throw sdl_error {SDL_GetError()};
        }
    }

    ~sdl() {
        if (cleanup_) {
            SDL_QuitSubSystem(systems);
            SDL_Quit();
        }
    }
private:
    bool cleanup_ {false};
};

//! RAII wrapper around SDL_Window
class sdl_window {
public:
    sdl_window()
      : handle_ {SDL_CreateWindow(
            "Boken"
           , SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
           , 1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)}
    {
        if (!handle_) {
            throw sdl_error {SDL_GetError()};
        }
    }

    operator SDL_Window*() const noexcept {
        return handle_.get();
    }
private:
    std::unique_ptr<SDL_Window, sdl_deleter_window> handle_;
};

//! RAII wrapper around SDL_Renderer
class sdl_renderer {
public:
    explicit sdl_renderer(sdl_window const& win)
      : handle_ {SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED
          | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE)}
    {
        if (!handle_) {
            throw sdl_error {SDL_GetError()};
        }

        if (SDL_SetRenderDrawBlendMode(handle_.get(), SDL_BLENDMODE_BLEND)) {
            throw sdl_error {SDL_GetError()};
        }
    }

    void set_draw_color(uint32_t const c) noexcept {
        SDL_SetRenderDrawColor(
            *this
          , static_cast<uint8_t>((c >>  0) & 0xFFu)
          , static_cast<uint8_t>((c >>  8) & 0xFFu)
          , static_cast<uint8_t>((c >> 16) & 0xFFu)
          , static_cast<uint8_t>((c >> 24) & 0xFFu));
    }

    operator SDL_Renderer*() const noexcept {
        return handle_.get();
    }
private:
    std::unique_ptr<SDL_Renderer, sdl_deleter_renderer> handle_;
};

//! RAII wrapper around SDL_Surface
class sdl_surface {
public:
    explicit sdl_surface(SDL_Surface* const ptr)
      : handle_ {ptr}
    {
        if (!handle_) {
            throw sdl_error {SDL_GetError()};
        }
    }

    operator SDL_Surface*() const noexcept {
        return handle_.get();
    }

    SDL_Surface* operator->() noexcept {
        return handle_.get();
    }

    SDL_Surface const* operator->() const noexcept {
        return handle_.get();
    }
private:
    std::unique_ptr<SDL_Surface, sdl_deleter_surface> handle_;
};

//! RAII wrapper around SDL_Texture
class sdl_texture {
public:
    explicit sdl_texture(SDL_Texture* const ptr)
      : handle_ {ptr}
    {
        if (!handle_
         || SDL_QueryTexture(*this, nullptr, nullptr, &width_, &height_)
        ) {
            throw sdl_error {SDL_GetError()};
        }
    }

    void set_color_mod(uint32_t const c) noexcept {
        SDL_SetTextureColorMod(
            *this
          , static_cast<uint8_t>((c >>  0) & 0xFFu)
          , static_cast<uint8_t>((c >>  8) & 0xFFu)
          , static_cast<uint8_t>((c >> 16) & 0xFFu));
    }

    operator SDL_Texture*() const noexcept {
        return handle_.get();
    }

    auto width()  const noexcept { return width_; }
    auto height() const noexcept { return height_; }
private:
    std::unique_ptr<SDL_Texture, sdl_deleter_texture> handle_;
    int width_  {};
    int height_ {};
};

sdl_texture create_font_texture(sdl_renderer& render) {
    auto converted = sdl_surface {
        SDL_ConvertSurfaceFormat(
            sdl_surface {SDL_LoadBMP("./data/tiles.bmp")}
          , SDL_PIXELFORMAT_RGBA8888
          , 0)};

    auto const w     = converted->w;
    auto const h     = converted->h;
    auto const pitch = converted->pitch;
    auto const bytes = converted->format->BytesPerPixel;
    auto const padding = pitch - w * bytes;

    auto const mask  = converted->format->Amask;
    auto const shift = converted->format->Ashift;

    auto it = reinterpret_cast<char*>(converted->pixels);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x, it += bytes) {
            auto p = reinterpret_cast<uint32_t*>(it);
            auto const color = *p & ~mask;
            *p = color | (color == 0 ? 0 : (0xFFu << shift));
        }
        it += padding;
    }

    auto result = sdl_texture {SDL_CreateTextureFromSurface(render, converted)};

    if (SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND)) {
        throw sdl_error {SDL_GetError()};
    }

    return result;
}

sdl_texture create_texture_from_file(sdl_renderer& render, string_view const filename) {
    auto result = sdl_texture {SDL_CreateTextureFromSurface(render
        , sdl_surface {SDL_LoadBMP(filename.data())})};

    if (!result) {
        throw sdl_error {SDL_GetError()};
    }

    if (SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND)) {
        throw sdl_error {SDL_GetError()};
    }

    return result;
}

template <typename T>
constexpr SDL_Rect make_sdl_rect_(axis_aligned_rect<T> const r) noexcept {
    return { value_cast(r.x0)
           , value_cast(r.y0)
           , value_cast(r.width())
           , value_cast(r.height())};
}

} //namespace

system::~system() = default;

class sdl_renderer_impl;

class sdl_system final : public system {
    friend class sdl_renderer_impl; //TODO remove this
public:
    sdl_system()
      : renderer_ {window_}
    {
        SDL_GetWindowSize(window_, &window_w_, &window_h_);

        handler_quit_         = [](    ) noexcept { return true; };
        handler_key_          = [](auto, auto) noexcept {};
        handler_mouse_move_   = [](auto, auto) noexcept {};
        handler_mouse_button_ = [](auto, auto) noexcept {};
        handler_mouse_wheel_  = [](auto, auto, auto) noexcept {};
        handler_text_input_   = [](auto) noexcept {};
    }

    static kb_modifiers get_key_mods() noexcept {
        return kb_modifiers {static_cast<uint32_t>(SDL_GetModState())};
    }

    void handle_event_mouse_button(SDL_MouseButtonEvent const& e) {
        using type = mouse_event::button_change_t;
        auto& m = last_mouse_event_;

        std::fill(begin(m.button_change), end(m.button_change), type::none);

        auto const b = (e.button > 0) && (e.button < mouse_event::button_count)
          ? static_cast<size_t>(e.button - 1)
          : mouse_event::button_count;

        if (b < mouse_event::button_count) {
            std::tie(m.button_change[b], m.button_state[b]) =
                (e.state == SDL_PRESSED) ? std::make_pair(type::went_down, true)
                                         : std::make_pair(type::went_up,   false);
        }

        last_mouse_event_.x  = e.x;
        last_mouse_event_.y  = e.y;
        last_mouse_event_.dx = 0;
        last_mouse_event_.dy = 0;

        handler_mouse_button_(m, get_key_mods());
    }

    void handle_event_mouse_move(SDL_MouseMotionEvent const& e) {
        auto& m = last_mouse_event_;

        std::fill(begin(m.button_change), end(m.button_change)
          , mouse_event::button_change_t::none);

        m.x  = e.x;
        m.y  = e.y;
        m.dx = e.xrel;
        m.dy = e.yrel;

        handler_mouse_move_(m, get_key_mods());
    }

    void handle_window_event(SDL_WindowEvent const& e) {
        //SDL_WINDOWEVENT_NONE,           /**< Never used */
        //SDL_WINDOWEVENT_SHOWN,          /**< Window has been shown */
        //SDL_WINDOWEVENT_HIDDEN,         /**< Window has been hidden */
        //SDL_WINDOWEVENT_EXPOSED,        /**< Window has been exposed and should be
        //                                     redrawn */
        //SDL_WINDOWEVENT_MOVED,          /**< Window has been moved to data1, data2
        //                                 */
        //SDL_WINDOWEVENT_RESIZED,        /**< Window has been resized to data1xdata2 */
        //SDL_WINDOWEVENT_SIZE_CHANGED,   /**< The window size has changed, either as a result of an API call or through the system or user changing the window size. */
        //SDL_WINDOWEVENT_MINIMIZED,      /**< Window has been minimized */
        //SDL_WINDOWEVENT_MAXIMIZED,      /**< Window has been maximized */
        //SDL_WINDOWEVENT_RESTORED,       /**< Window has been restored to normal size
        //                                     and position */
        //SDL_WINDOWEVENT_ENTER,          /**< Window has gained mouse focus */
        //SDL_WINDOWEVENT_LEAVE,          /**< Window has lost mouse focus */
        //SDL_WINDOWEVENT_FOCUS_GAINED,   /**< Window has gained keyboard focus */
        //SDL_WINDOWEVENT_FOCUS_LOST,     /**< Window has lost keyboard focus */
        //SDL_WINDOWEVENT_CLOSE           /**< The window manager requests that the
        //                                     window be closed */

        switch (e.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED :
            break;
        case SDL_WINDOWEVENT_RESIZED :
            window_w_ = e.data1;
            window_h_ = e.data2;
            break;
        default:
            break;
        }
    }
public:
    //===---------------------overridden functions---------------------------===

    void on_request_quit(on_request_quit_handler handler) final override {
        handler_quit_ = std::move(handler);
    }

    void on_key(on_key_handler handler) final override {
        handler_key_ = std::move(handler);
    }

    void on_mouse_move(on_mouse_move_handler handler) final override {
        handler_mouse_move_ = std::move(handler);
    }

    void on_mouse_button(on_mouse_button_handler handler) final override {
        handler_mouse_button_ = std::move(handler);
    }

    void on_mouse_wheel(on_mouse_wheel_handler handler) final override {
        handler_mouse_wheel_ = std::move(handler);
    }

    void on_text_input(on_text_input_handler handler) final override {
        handler_text_input_ = std::move(handler);
    }

    bool is_running() final override {
        return running_;
    }

    int do_events() final override;

    recti32 get_client_rect() const final override {
        int w = 0;
        int h = 0;
        SDL_GL_GetDrawableSize(window_, &w, &h);
        return {point2i32 {}, sizei32x {w}, sizei32y {h}};
    }
private:
    on_request_quit_handler handler_quit_;
    on_key_handler          handler_key_;
    on_mouse_move_handler   handler_mouse_move_;
    on_mouse_button_handler handler_mouse_button_;
    on_mouse_wheel_handler  handler_mouse_wheel_;
    on_text_input_handler   handler_text_input_;

    mouse_event last_mouse_event_ {};

    sdl          sdl_;
    sdl_window   window_;
    sdl_renderer renderer_;

    int window_w_ {};
    int window_h_ {};

    bool running_ = true;
};

int sdl_system::do_events() {
    int count = 0;

    for (SDL_Event event; SDL_PollEvent(&event); ++count) {
        switch (event.type) {
        case SDL_WINDOWEVENT :
            handle_window_event(event.window);
            break;
        case SDL_QUIT:
            running_ = !handler_quit_();
            break;
        case SDL_TEXTINPUT :
            handler_text_input_(text_input_event {
                event.text.timestamp
              , event.text.text});
            break;
        case SDL_KEYDOWN :
        case SDL_KEYUP :
            handler_key_(kb_event {
                event.key.timestamp
                , static_cast<kb_scancode>(event.key.keysym.scancode)
                , static_cast<kb_keycode>(event.key.keysym.sym)
                , event.key.keysym.mod
                , !!event.key.repeat
                , event.key.state == SDL_PRESSED
            }, kb_modifiers {event.key.keysym.mod});
            break;
        case SDL_MOUSEMOTION :
            handle_event_mouse_move(event.motion);
            break;
        case SDL_MOUSEBUTTONDOWN :
        case SDL_MOUSEBUTTONUP :
            handle_event_mouse_button(event.button);
            break;
        case SDL_MOUSEWHEEL :
            handler_mouse_wheel_(event.wheel.y, event.wheel.x, get_key_mods());
            break;
        default:
            break;
        }
    }

    return count;
}

std::unique_ptr<system> make_system() {
    return std::make_unique<sdl_system>();
}

/////////////////////////////////

renderer2d::~renderer2d() = default;

class sdl_renderer_impl final : public renderer2d {
public:
    sdl_renderer_impl(system& sys);
//------------------------------------------------------------------------------
    recti32 get_client_rect() const final override {
        return sys_.get_client_rect();
    }

    void set_clip_rect(recti32 const r) final override {
        if (r == recti32 {}) {
            clip_rect();
            return;
        }

        auto const r0 = make_sdl_rect_(r);
        if (SDL_RenderSetClipRect(r_, &r0)) {
            throw sdl_error {SDL_GetError()};
        }
    }

    undo_clip_rect clip_rect(recti32 const r) final override {
        auto const prev = clip_rect_;
        set_clip_rect(r);
        clip_rect_ = r;
        return {*this, prev};
    }

    void clip_rect() final override {
        if (SDL_RenderSetClipRect(r_, nullptr)) {
            throw sdl_error {SDL_GetError()};
        }

        clip_rect_ = recti32 {};
    }

    void set_transform(transform_t const t) final override {
        SDL_RenderSetScale(r_, t.scale_x, t.scale_y);
        trans_ = t;
    }

    undo_transform transform(transform_t const t) final override {
        auto const prev = trans_;
        set_transform(t);
        return {*this, prev};
    }

    void transform() final override {
        transform({1.0f, 1.0f, 0.0f, 0.0f}).dismiss();
    }

    void render_clear() final override {
        SDL_SetRenderDrawColor(r_, 127, 127, 0, 255);
        SDL_RenderClear(r_);
    }
    void render_present() final override {
        SDL_RenderPresent(r_);
    }

    void fill_rect(recti32 const r, uint32_t const color) final override {
        fill_rects(&r, &r + 1, color);
    }

    void fill_rects(
        recti32  const* const r_first, recti32  const* const r_last
      , uint32_t const* const c_first, uint32_t const* const c_last
    ) final override {
        BK_ASSERT(std::distance(r_first, r_last)
               == std::distance(c_first, c_last));

        uint32_t last_color = 0;
        r_.set_draw_color(last_color);

        fill_rects_impl(r_first, r_last, [&, it = c_first]() mutable {
            auto const c = *it++;
            if (c != last_color) {
                r_.set_draw_color(last_color = c);
            }
        });
    }

    void fill_rects(
        recti32  const* const r_first, recti32  const* const r_last
      , uint32_t const color
    ) final override {
        r_.set_draw_color(color);
        fill_rects_impl(r_first, r_last, [] {});
    }

    void draw_rect(recti32 const r, int32_t const border_size, uint32_t const color) final override {
        draw_rects(&r, &r + 1, color, border_size);
    }

    void draw_rects(
        recti32  const* const r_first, recti32  const* const r_last
      , uint32_t const color
      , int32_t  const border_size
    ) final override {
        r_.set_draw_color(color);
        draw_rects_impl(r_first, r_last, border_size, [] {});
    }

    void draw_rects(
        recti32  const* const r_first, recti32  const* const r_last
      , uint32_t const* const c_first, uint32_t const* const c_last
      , int32_t  const border_size
    ) final override {
        BK_ASSERT(std::distance(r_first, r_last)
               == std::distance(c_first, c_last));

        uint32_t last_color = 0;
        r_.set_draw_color(last_color);

        draw_rects_impl(r_first, r_last, border_size, [&, it = c_first]() mutable {
            auto const c = *it++;
            if (c != last_color) {
                r_.set_draw_color(last_color = c);
            }
        });
    }

    void draw_background() final override;

    void draw_tiles(tile_params_uniform const& p) final override {
        BK_ASSERT(p.count >= 0
               && p.texture_id < textures_.size());

        auto&               texture    = textures_[p.texture_id];
        SDL_Texture*  const tex_handle = texture;
        SDL_Renderer* const renderer   = r_;

        uint32_t last_color = 0;
        texture.set_color_mod(last_color);

        auto const tx = ceil_as<int>(trans_.trans_x / trans_.scale_x);
        auto const ty = ceil_as<int>(trans_.trans_y / trans_.scale_y);
        auto const w  = value_cast(p.tile_w);
        auto const h  = value_cast(p.tile_h);

        BK_ASSERT(w >= 0 && h >= 0);

        auto p_xy = p.pos_coords;
        auto p_st = p.tex_coords;
        auto p_c  = p.colors;

        auto const n = static_cast<size_t>(p.count);
        for (size_t i = 0; i < n; ++i, ++p_xy, ++p_st, ++p_c) {
            auto const xy    = p_xy.value<point2i16>();
            auto const st    = p_st.value<point2i16>();
            auto const color = p_c.value<uint32_t>();

            if (color != last_color) {
                texture.set_color_mod(last_color = color);
            }

            SDL_Rect src {value_cast(st.x),      value_cast(st.y),      w, h};
            SDL_Rect dst {value_cast(xy.x) + tx, value_cast(xy.y) + ty, w, h};

            SDL_RenderCopy(renderer, tex_handle, &src, &dst);
        }
    }

    void draw_tiles(tile_params_variable const& p) final override {
        BK_ASSERT(p.count >= 0
               && p.texture_id < textures_.size());

        auto&               texture    = textures_[p.texture_id];
        SDL_Texture*  const tex_handle = texture;
        SDL_Renderer* const renderer   = r_;

        uint32_t last_color = 0;
        texture.set_color_mod(last_color);

        auto const tx = ceil_as<int>(trans_.trans_x / trans_.scale_x);
        auto const ty = ceil_as<int>(trans_.trans_y / trans_.scale_y);

        auto p_xy = p.pos_coords;
        auto p_st = p.tex_coords;
        auto p_wh = p.tex_sizes;
        auto p_c  = p.colors;

        auto const n = static_cast<size_t>(p.count);
        for (size_t i = 0; i < n; ++i, ++p_xy, ++p_st, ++p_wh, ++p_c) {
            auto const xy    = p_xy.value<point2i16>();
            auto const st    = p_st.value<point2i16>();
            auto const wh    = p_wh.value<point2i16>();
            auto const w     = value_cast(wh.x);
            auto const h     = value_cast(wh.y);
            auto const color = p_c.value<uint32_t>();

            if (color != last_color) {
                texture.set_color_mod(last_color = color);
            }

            SDL_Rect src {value_cast(st.x),      value_cast(st.y),      w, h};
            SDL_Rect dst {value_cast(xy.x) + tx, value_cast(xy.y) + ty, w, h};

            SDL_RenderCopy(renderer, tex_handle, &src, &dst);
        }
    }

//------------------------------------------------------------------------------

    template <typename FwdIt, typename SetColor>
    void fill_rects_impl(FwdIt const first, FwdIt const last, SetColor c) {
        for (auto it = first; it != last; ++it) {
            c();

            auto const r = make_sdl_rect_(*it);
            SDL_RenderFillRect(r_, &r);
        }
    }

    template <typename FwdIt, typename SetColor>
    void draw_rects_impl(FwdIt const first, FwdIt const last, int32_t const border_size, SetColor c) {
        auto const tx = ceil_as<int>(trans_.trans_x / trans_.scale_x);
        auto const ty = ceil_as<int>(trans_.trans_y / trans_.scale_y);

        auto const w  = border_size;
        auto const w2 = 2 * w;
        auto const h  = border_size;

        for (auto it = first; it != last; ++it) {
            auto const r = *it;

            auto const rw = value_cast(r.width());
            auto const rh = value_cast(r.height());

            auto const x0 = value_cast(r.x0) + tx;
            auto const y0 = value_cast(r.y0) + ty;
            auto const x1 = value_cast(r.x1) + tx;
            auto const y1 = value_cast(r.y1) + ty;

            constexpr int count = 4;
            SDL_Rect const rects[count] {
                {x0 + 0, y0 + 0,       w, rh}
              , {x1 - w, y0 + 0,       w, rh}
              , {x0 + w, y0 + 0, rw - w2, h }
              , {x0 + w, y1 - h, rw - w2, h }
            };

            c();
            SDL_RenderFillRects(r_, rects, count);
        }
    }

private:
    sdl_system&   sys_;
    sdl_renderer& r_;

    std::vector<sdl_texture> textures_;

    transform_t trans_ {1.0f, 1.0f, 0.0f, 0.0f};
    recti32     clip_rect_;
};

std::unique_ptr<renderer2d> make_renderer(system& sys) {
    return std::make_unique<sdl_renderer_impl>(sys);
}

sdl_renderer_impl::sdl_renderer_impl(system& sys)
  : sys_ {dynamic_cast<sdl_system&>(sys)}
  , r_   {sys_.renderer_}
{
    textures_.reserve(4);

    //base
    textures_.push_back(
        create_texture_from_file(r_, "./data/tiles.bmp"));
    //entities
    textures_.push_back(
        create_texture_from_file(r_, "./data/entities.bmp"));
    //items
    textures_.push_back(
        create_texture_from_file(r_, "./data/tiles.bmp"));
    //font
    textures_.push_back(create_font_texture(r_));
    //background
    textures_.push_back(
        create_texture_from_file(r_, "./data/background.bmp"));

}

void sdl_renderer_impl::draw_background() {
    auto const client = get_client_rect();

    auto const& bg = textures_[4];

    auto const w = bg.width();
    auto const h = bg.height();
    auto const ww = value_cast(client.width());
    auto const wh = value_cast(client.height());

    auto const tx  = (ww + w - 1) / w;
    auto const ty  = (wh + h - 1) / h;

    SDL_Rect r {0, 0, w, h};

    for (int y = 0; y < ty; ++y, r.y += h) {
        r.x = 0;
        for (int x = 0; x < tx; ++x, r.x += w) {
            SDL_RenderCopy(r_, bg, nullptr, &r);
        }
    }
}

} //namespace boken
