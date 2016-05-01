#include "system.hpp"           // for read_only_pointer_t, etc
#include "math.hpp"             // for ceil_as

#include <bkassert/assert.hpp>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <functional>           // for function
#include <memory>               // for unique_ptr
#include <stdexcept>            // for runtime_error
#include <tuple>                // for tie, tuple
#include <utility>              // for move, pair, swap

#include <cstdint>              // for uint16_t, uint8_t, uint32_t

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

    sdl() {
        if (SDL_WasInit(0) & systems) {
            throw sdl_error {"SDL already initialized"};
        }

        if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
            throw sdl_error {SDL_GetError()};
        }
    }

    sdl(sdl&& other) noexcept
      : cleanup_ {other.cleanup_}
    {
        other.cleanup_ = false;
    }

    sdl& operator=(sdl&& rhs) noexcept {
        std::swap(cleanup_, rhs.cleanup_);
        return *this;
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

} //namespace

namespace boken {

system::~system() = default;

class sdl_system final : public system {
public:
    sdl_system()
      : renderer_ {window_}
      , base_tiles_ {SDL_CreateTextureFromSurface(
          renderer_, sdl_surface {SDL_LoadBMP("./data/tiles.bmp")})}
      , entity_tiles_ {SDL_CreateTextureFromSurface(
          renderer_, sdl_surface {SDL_LoadBMP("./data/entities.bmp")})}
      , item_tiles_ {SDL_CreateTextureFromSurface(
          renderer_, sdl_surface {SDL_LoadBMP("./data/tiles.bmp")})}
      , font_cache_ {create_font_texture(renderer_)}
      , background_{SDL_CreateTextureFromSurface(
          renderer_, sdl_surface {SDL_LoadBMP("./data/background.bmp")})}
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

    void set_draw_color(uint32_t const c) noexcept {
        SDL_SetTextureColorMod(
            *active_texture_
          , static_cast<uint8_t>((c >>  0) & 0xFFu)
          , static_cast<uint8_t>((c >>  8) & 0xFFu)
          , static_cast<uint8_t>((c >> 16) & 0xFFu));

        SDL_SetRenderDrawColor(
            renderer_
          , static_cast<uint8_t>((c >>  0) & 0xFFu)
          , static_cast<uint8_t>((c >>  8) & 0xFFu)
          , static_cast<uint8_t>((c >> 16) & 0xFFu)
          , static_cast<uint8_t>((c >> 24) & 0xFFu));
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
    //
    // overridden functions
    //

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

    recti32 render_get_client_rect() const final override {
        SDL_Rect r {};
        SDL_RenderGetViewport(renderer_, &r);
        return {point2i32 {}, sizei32x {r.w}, sizei32y {r.h}};
    }

    void render_clear() final override {
        SDL_SetRenderDrawColor(renderer_, 127, 127, 0, 255);
        SDL_RenderClear(renderer_);
    }

    void render_present() final override {
        SDL_RenderPresent(renderer_);
    }

    template <typename T>
    static SDL_Rect make_sdl_rect_(axis_aligned_rect<T> const r) noexcept {
        return { value_cast(r.x0)
               , value_cast(r.y0)
               , value_cast(r.width())
               , value_cast(r.height())};
    }

    void render_set_clip_rect(recti32 const r) final override {
        auto const r0 = make_sdl_rect_(r);
        if (SDL_RenderSetClipRect(renderer_, &r0)) {
            throw sdl_error {SDL_GetError()};
        }
    }

    void render_clear_clip_rect() final override {
        if (SDL_RenderSetClipRect(renderer_, nullptr)) {
            throw sdl_error {SDL_GetError()};
        }
    }

    void render_fill_rect(recti32 const r, uint32_t const color) final override {
        set_draw_color(color);

        auto const r0 =  make_sdl_rect_(r);
        SDL_RenderFillRect(renderer_, &r0);
    }

    void render_draw_rect(recti32 const r, int const w, int h, uint32_t const color) final override {
        auto const rw = value_cast(r.width());
        auto const rh = value_cast(r.height());

        auto const tx = ceil_as<int>(tx_ / sx_);
        auto const ty = ceil_as<int>(ty_ / sy_);

        auto const x0 = value_cast(r.x0) + tx;
        auto const y0 = value_cast(r.y0) + ty;
        auto const x1 = value_cast(r.x1) + tx;
        auto const y1 = value_cast(r.y1) + ty;

        constexpr int count = 4;
        SDL_Rect const rects[count] {
            {x0 + 0, y0 + 0,        w, rh}
          , {x1 - w, y0 + 0,        w, rh}
          , {x0 + w, y0 + 0, rw - 2*w, h }
          , {x0 + w, y1 - h, rw - 2*w, h }
        };

        set_draw_color(color);
        SDL_RenderFillRects(renderer_, rects, count);
    }

    void render_draw_rect(recti32 const r, int const w, uint32_t const color) final override {
        render_draw_rect(r, w, w, color);
    }

    void render_background() final override {
        auto const w = background_.width();
        auto const h = background_.height();

        auto const tx  = (window_w_ + w - 1) / w;
        auto const ty  = (window_h_ + h - 1) / h;

        SDL_Rect r {0, 0, w, h};

        for (int y = 0; y < ty; ++y, r.y += h) {
            r.x = 0;
            for (int x = 0; x < tx; ++x, r.x += w) {
                SDL_RenderCopy(renderer_, background_, nullptr, &r);
            }
        }
    }

    void render_set_texture(uint32_t const id) final override {
        switch (id) {
        case 0  : break;
        case 1  : active_texture_ = &entity_tiles_; return;
        case 2  : active_texture_ = &item_tiles_;   return;
        case 3  : active_texture_ = &font_cache_;   return;
        default : break;
        }

        active_texture_ = &base_tiles_;
    }

    void render_set_data(
        render_data_type    const type
      , read_only_pointer_t const data
    ) noexcept final override {
        switch (type) {
        case render_data_type::position : position_data_ = data; break;
        case render_data_type::texture  : texture_data_  = data; break;
        case render_data_type::color    : color_data_    = data; break;
        default:
            BK_ASSERT_SAFE(false);
            break;
        }
    }

    void render_set_tile_size(sizei32x const w, sizei32y const h) noexcept final override {
        tile_w_ = value_cast(w);
        tile_h_ = value_cast(h);
    }

    void render_set_transform(float const sx, float const sy, float const tx, float const ty) noexcept final override {
        sx_ = sx;
        sy_ = sy;
        tx_ = tx;
        ty_ = ty;

        SDL_RenderSetScale(renderer_, sx_, sy_);
    }

    void render_data_n(size_t const n) noexcept final override {
        auto pd = position_data_;
        auto td = texture_data_;
        auto cd = color_data_;

        SDL_Rect src {0, 0, tile_w_, tile_h_};
        SDL_Rect dst {0, 0, ceil_as<int>(tile_w_), ceil_as<int>(tile_h_)};

        uint32_t last_color = 0;
        set_draw_color(last_color);

        auto const tx = ceil_as<int>(tx_ / sx_);
        auto const ty = ceil_as<int>(ty_ / sy_);

        SDL_Texture*  const texture  = *active_texture_;
        SDL_Renderer* const renderer = renderer_;

        for (size_t i = 0; i < n; ++i, ++pd, ++td, ++cd) {
            std::tie(src.x, src.y) = td.value<std::pair<uint16_t, uint16_t>>();
            std::tie(dst.x, dst.y) = pd.value<std::pair<uint16_t, uint16_t>>();

            dst.x = dst.x + tx;
            dst.y = dst.y + ty;

            auto const color = cd.value<uint32_t>();
            if (color != last_color) {
                set_draw_color(last_color = color);
            }

            SDL_RenderCopy(renderer, texture, &src, &dst);
        }
    }
private:
    on_request_quit_handler handler_quit_;
    on_key_handler          handler_key_;
    on_mouse_move_handler   handler_mouse_move_;
    on_mouse_button_handler handler_mouse_button_;
    on_mouse_wheel_handler  handler_mouse_wheel_;
    on_text_input_handler   handler_text_input_;

    mouse_event last_mouse_event_ {};

    bool running_ = true;

    sdl          sdl_;
    sdl_window   window_;
    sdl_renderer renderer_;
    sdl_texture  base_tiles_;
    sdl_texture  entity_tiles_;
    sdl_texture  item_tiles_;
    sdl_texture  font_cache_;
    sdl_texture  background_;

    sdl_texture* active_texture_ {&base_tiles_};

    read_only_pointer_t position_data_;
    read_only_pointer_t texture_data_;
    read_only_pointer_t color_data_;

    int window_w_ {};
    int window_h_ {};

    int tile_w_ {};
    int tile_h_ {};

    float sx_ = 1.0f;
    float sy_ = 1.0f;
    float tx_ = 0.0f;
    float ty_ = 0.0f;
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

} //namespace boken
