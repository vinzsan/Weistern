#include <iostream>
#include <memory>
#include <raylib.h>
#include <atomic>
#include <stdexcept>

#define WIDTH 800
#define HEIGHT 600

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
  SpriteAnim anim;

  Vector2 pos{100,100};
  bool facing = true;
  int speed = 90;

  bool walking = false;
  bool lock_anim = false;
  SpriteAnimWalk anim_state = SpriteAnimWalk::Idle;
  SpriteAnimWalk last_state = SpriteAnimWalk::Idle;

public:

  MainState(struct RayMetadata &m) : mdata(m) {
    anim.texture = LoadTexture("./assets/char_black.png");
    anim.fwidth = 128;
    anim.fheight = 128;
    anim.frame = 8;
    anim.row = 14;
    anim.speed = 0.1f;
    
    SetTextureFilter(anim.texture, TEXTURE_FILTER_POINT);
  }

  void rendering() override {
    s_width = GetScreenWidth();s_height = GetScreenHeight();

    ClearBackground(GRAY);
    /*DrawRectanglePro(this->r1,Vector2 {0.0,0.0},0.0,GREEN);*/

    constexpr float box = 10;
    DrawTextCentered(mdata.font,"This is main?",20,0.0,WHITE);
    DrawAnim(anim,pos,facing,0.50f);
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
        if(facing) {
          pos.x += 5;
        } else {
          pos.x -= 5;
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
    
    pos.x = clamp(pos.x,0.0f,GetScreenWidth() - anim.fwidth * 0.50f);
    pos.y = clamp(pos.y,0.0f,GetScreenHeight() - anim.fheight * 0.50f);
    
    UpdateAnim(anim);
  }

  void event() override {
    walking = false;
    if(IsKeyDown(KeyboardKey::KEY_RIGHT)) {
      pos.x += speed * GetFrameTime();
      facing = true;
      walking = true;
    }
    if(IsKeyDown(KeyboardKey::KEY_LEFT)){
      pos.x -= speed * GetFrameTime();
      facing = false;
      walking = true;
    }
    if(IsKeyPressed(KeyboardKey::KEY_S) && !lock_anim){
      anim_state = SpriteAnimWalk::Teleport;
      lock_anim = true;
    }

    if(!lock_anim) anim_state = walking ? SpriteAnimWalk::Walk : SpriteAnimWalk::Idle;

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
    DrawText("Hello from menu",100,100,20,WHITE);
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