#include <iostream>
#include <memory>
#include <atomic>
#include <stdexcept>

#include "../include/raylib.h"
#include "../include/raymath.h"

#define WIDTH 850
#define HEIGHT 400

enum SceneState {
  Scene1,
  Scene2,
  None
};

enum SpriteAnimWalk {
  Walk,
  Teleport,
  Idle
};

enum class BackgroundMode {
  Cover,
  Contain,
  None
};

/*  struct Base Metadata  */

struct RayMetadata {
  Font font;
  
  RayMetadata() = default;
  
  int load(const std::string &name_file){
    font = LoadFont(name_file.c_str());
    if(font.texture.id == 0) {
      std::cerr << "INFO : failed to open file path " << name_file << std::endl;
      throw std::runtime_error("ERROR : exception file");
    }
    return 0;
  }
  ~RayMetadata() {
    if(font.texture.id != 0) UnloadFont(font);
    std::cout << "INFO : destructor debug RayMetadata" << std::endl;
  }

  RayMetadata(const RayMetadata &) = delete;
  RayMetadata& operator=(const RayMetadata &) = delete;
};

/*  End Base Metadata */

/*  struct SpriteAnim  */

struct SpriteAnim {
  Texture2D texture;
  int fwidth;
  int fheight;
  int frame;
  int row;
  int current = 0;
  float timer = 0.0f;
  float speed = 0.1f;

  SpriteAnim() = default;
  ~SpriteAnim(){
    if(texture.id != 0) UnloadTexture(texture);
    std::cout << "INFO : Texture Unloaded" << std::endl;
  }
};

/*  End SpriteAnim  */

/*  struct BackgroundRolling  */

struct BackgroundRolling {
  Texture2D texture;
  float x = 0.0f;
  float speed = 60.0f;

  
};

/*  End BackgroundRolling  */

/*  Function State  */

void DrawTextCentered(Font font,const std::string &str,int size,float spacing,Color color){
  Vector2 t_size = MeasureTextEx(font,str.c_str(),static_cast<float>(size),spacing);
  
  Vector2 pos {
    (GetScreenWidth() - t_size.x) / 2,
    (GetScreenHeight() - t_size.y) / 2,
  };

  DrawTextEx(
    font,str.c_str(),pos,static_cast<float>(size),spacing,color
  );
}

void UpdateAnim(SpriteAnim &m){
  m.timer += GetFrameTime();
  if(m.timer >= m.speed){
    m.timer -= m.speed;
    m.current = (m.current + 1) % m.frame;
  }
}

void DrawAnim(const SpriteAnim &a,Vector2 pos,bool facing,float scale){
  Rectangle src {
    static_cast<float>(a.current * a.fwidth),
    static_cast<float>(a.row * a.fheight),
    facing ? static_cast<float>(a.fwidth) : -static_cast<float>(a.fwidth),
    static_cast<float>(a.fheight)
  };

  Rectangle dst {
    pos.x,pos.y,
    a.fwidth * scale,
    a.fheight * scale
  };

  Vector2 origin {
    dst.width * 0.5f,
    dst.height
  };

  DrawTexturePro(a.texture,src,dst,origin,0.0f,WHITE);
}

template <typename T>
static inline T clamp(T v,T min,T max){
  if(v < min) return min;
  if(v > max) return max;
  return v;
}

void DrawBackgroundRolling(BackgroundRolling &bg, BackgroundMode bmode){
    float scale = (bmode == BackgroundMode::Cover) ? std::max(GetScreenWidth() / (float)bg.texture.width, GetScreenHeight() / (float)bg.texture.height)
      : std::min(GetScreenWidth() / (float)bg.texture.width, GetScreenHeight() / (float)bg.texture.height);

    float x = bg.x;
    float y = (GetScreenHeight() - bg.texture.height * scale) / 2.0f;

    DrawTexturePro(
        bg.texture,
        {0,0,(float)bg.texture.width,(float)bg.texture.height},
        {x, y, bg.texture.width * scale, bg.texture.height * scale},
        {0,0},
        0.0f,
        WHITE
    );

    if(x < 0) {
        DrawTexturePro(
            bg.texture,
            {0,0,(float)bg.texture.width,(float)bg.texture.height},
            {x + bg.texture.width * scale, y, bg.texture.width * scale, bg.texture.height * scale},
            {0,0},
            0.0f,
            WHITE
        );
    } else if(x > 0) {
        DrawTexturePro(
            bg.texture,
            {0,0,(float)bg.texture.width,(float)bg.texture.height},
            {x - bg.texture.width * scale, y, bg.texture.width * scale, bg.texture.height * scale},
            {0,0},
            0.0f,
            WHITE
        );
    }
}

/*  End Function State  */

class State {
protected:

  SceneState scenestate = SceneState::None;

public:

  State() = default;
  virtual ~State() = default;
  
  virtual void rendering() = 0;
  virtual void update() = 0;
  virtual void event() = 0;

  SceneState route() const {
    return this->scenestate;
  }

  void reset_scene() {
    scenestate = SceneState::None;
  }

};

class MainState : public State {
protected:

  struct RayMetadata &mdata;
  Rectangle r1 = {100,100,300,200};
  int s_width,s_height;
  struct SpriteAnim anim;
  struct BackgroundRolling bgrolling;
  

  Vector2 pos{0,0};
  bool facing = true;
  int speed = 90;
  float movement = 0.0f;
  bool show_text = false;

  bool walking = false;
  bool lock_anim = false;
  SpriteAnimWalk anim_state = SpriteAnimWalk::Idle;
  SpriteAnimWalk last_state = SpriteAnimWalk::Idle;

public:

  MainState(struct RayMetadata &m) : mdata(m) {
    anim.texture = LoadTexture("./assets/char_black.png");
    if(anim.texture.id == 0){
      throw std::runtime_error("failed to open assets file");
    }
    
    anim.fwidth = 128;
    anim.fheight = 128;
    anim.frame = 8;
    anim.row = 14;
    anim.speed = 0.1f;
    
    bgrolling.texture = LoadTexture("./assets/assets_bgrollin.jpg");
    if(bgrolling.texture.id == 0){
      throw std::runtime_error("failed to open assets file");
    }

    bgrolling.x = -(bgrolling.texture.width - GetScreenWidth()) / 2.0f;
    bgrolling.speed = 180.0f;
    
    pos.x = (GetScreenWidth() - anim.fwidth * 0.5f) / 2.0f;
    pos.y = (GetScreenHeight() - anim.fheight * 0.5f) / 2.0f;
    
    SetTextureFilter(anim.texture, TEXTURE_FILTER_POINT);
  }

  void rendering() override {
    s_width = GetScreenWidth();s_height = GetScreenHeight();

    ClearBackground(GRAY);
    /*DrawRectanglePro(this->r1,Vector2 {0.0,0.0},0.0,GREEN);*/

    /* [warn(dead_code)] */
    float scaleX = static_cast<float>(s_width) / bgrolling.texture.width;
    float scaleY = static_cast<float>(s_height) / bgrolling.texture.height;

    float scale = std::max(scaleX, scaleY);
    float bg_w_scaled = bgrolling.texture.width * scale;
    float bg_h_scaled = bgrolling.texture.height * scale;

    float bg_x = bgrolling.x - (bg_w_scaled - s_width) / 2.0f;
    float bg_y = - (bg_h_scaled - s_height) / 2.0f;

    Rectangle src { 0, 0, static_cast<float>(bgrolling.texture.width), static_cast<float>(bgrolling.texture.height) };
    Rectangle dst { bg_x, bg_y, bg_w_scaled, bg_h_scaled };
    Vector2 origin {0, 0};

    DrawBackgroundRolling(bgrolling,BackgroundMode::Cover);

    constexpr float box = 10;

    pos.y = s_height - anim.fheight * 0.2f - 2;
    DrawAnim(anim,pos,facing,0.50f);
    
    DrawTextPro(mdata.font,"[Untracked : Vinz?] [ Use 'E' to see hints ]",
      Vector2 {10,10},Vector2 {0,0},0.0,20.0f,0.2f,GRAY
    );
    
    if(show_text){
      DrawRectangle(0, s_height * 0.10f, s_width, s_height * 0.10f, Color {255,255,255,20});
      DrawTextEx(mdata.font,"[Apis] : What is this thing..? ,i thought it never happend before..",
        Vector2 { 10,s_height * 0.10f},20.0f,0.0,YELLOW
      );
    }
  }

  void update() override {
    if(anim_state == SpriteAnimWalk::Walk){
      anim.row = 0;
      anim.frame = 9;
      anim.speed = 0.1f;
    } else if(anim_state == SpriteAnimWalk::Teleport){
      anim.row = 13;
      anim.frame = 12;
      anim.speed = 0.1f;
      
      if(lock_anim) { // Test speed teleport
        if(!facing) {
          bgrolling.x += 5;
        } else {
          bgrolling.x -= 5;
        }
      }
      if(anim.current == anim.frame - 1){
        lock_anim = false;
        anim_state = Idle;
      }
    } else {
      anim.row = 3;
      anim.frame = 8;
      anim.speed = 0.1f;
    }
    
    if(anim_state != last_state){
      anim.current = 0;
      anim.timer = 0.0f;
      last_state = anim_state;
    }
    
    /*
    float left_bound  = 300.0f;
    float right_bound = GetScreenWidth() - 300.0f - anim.fwidth * 0.5f;
    pos.x = clamp(pos.x, left_bound, right_bound);

    if(pos.x <= left_bound && movement > 0.0f){
      bgrolling.x += movement;
    } else if(pos.x >= right_bound && movement < 0.0f){
      bgrolling.x += movement;
    }
    */
    
    UpdateAnim(anim);
  }

  void event() override {
    walking = false;
    movement = 0.0f;

    float frametime = GetFrameTime();
    if(IsKeyDown(KeyboardKey::KEY_RIGHT)) {
      //pos.x += speed * frametime;
      movement -= bgrolling.speed * frametime;
      facing = true;
      walking = true;
    }
    if(IsKeyDown(KeyboardKey::KEY_LEFT)){
      //pos.x -= speed * frametime;
      movement += bgrolling.speed * frametime;
      facing = false;
      walking = true;
    }
    if(IsKeyPressed(KeyboardKey::KEY_S) && !lock_anim){
      anim_state = SpriteAnimWalk::Teleport;
      lock_anim = true;
    }

    if(IsKeyPressed(KeyboardKey::KEY_R)){
      show_text = true;
    }

    if(walking) show_text = false;

    if(!lock_anim) anim_state = walking ? SpriteAnimWalk::Walk : SpriteAnimWalk::Idle;
    bgrolling.x += movement;

    float scale = std::max(GetScreenWidth() / (float)bgrolling.texture.width,
                           GetScreenHeight() / (float)bgrolling.texture.height);
    float bg_w_scaled = bgrolling.texture.width * scale;

    if(bgrolling.x <= -bg_w_scaled) bgrolling.x += bg_w_scaled;
    if(bgrolling.x >= 0) bgrolling.x -= bg_w_scaled;

    if(IsKeyPressed(KeyboardKey::KEY_A)) {
      std::cout << "Key A pressed" << std::endl;
    }
    if(IsKeyPressed(KeyboardKey::KEY_E)) {
      this->scenestate = Scene2;
    }
  }
};

class MenuState : public State {
public:

  void rendering() override {
    ClearBackground(BLACK);
    DrawText("Menu State : use 'E' to return",100,100,20,WHITE);

    DrawText("Tips : use 'S' to dash with other animation :3",100,150,15,WHITE);
  }
  
  void update() override {

  }

  void event() override {
    if(IsKeyPressed(KeyboardKey::KEY_E)) {
      this->scenestate = SceneState::Scene1;
    }
  }
};

class RayGame {
private:

  struct RayMetadata raymetadata;
  
  std::atomic<bool> counter = ATOMIC_VAR_INIT(0);
  std::unique_ptr<State> state;
  SceneState next = SceneState::None;

public:

  RayGame() {
    InitWindow(WIDTH,HEIGHT,"Test");
    SetTargetFPS(60);
    
    raymetadata.load("./assets/MapleMono-NF-Regular.ttf");

    state = std::make_unique<MainState>(raymetadata);
    counter.store(true,std::memory_order_relaxed);
  }
  
  ~RayGame(){
    std::cout << "Closed" << std::endl;
  }
  
  void run(){
    while(!WindowShouldClose() && counter.load(std::memory_order_acquire)){
      state->event();
      
      if(IsKeyPressed(KeyboardKey::KEY_Q)){
        counter.store(false,std::memory_order_relaxed);
      }

      state->update();
      this->next = state->route();

      if(next != SceneState::None){
        switch(next){
          case Scene1:
            state = std::make_unique<MainState>(raymetadata);
            break;
          case Scene2:
            state = std::make_unique<MenuState>();
            break;
          default:
            break;
        }
        state->reset_scene();
      }

      BeginDrawing();
      state->rendering();
      EndDrawing();
    }
  }
};

int main(){
  std::unique_ptr<RayGame> g = std::make_unique<RayGame>();
  g->run();
  return 0;
}