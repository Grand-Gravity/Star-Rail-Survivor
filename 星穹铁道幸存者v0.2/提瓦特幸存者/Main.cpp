#include<graphics.h>
#include<string>
#include<vector>
using namespace std;
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"MSIMG32.LIB")
#include<iostream>

//此处可调帧率vvv
const int fps = 60, fr_intv = 1000 / fps;
const int WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720;
const int BUTTON_WIDTH = 192, BUTTON_HEIGHT = 75;
int idx_current_anim = 0;
IMAGE img_background, img_menu, img_ice;
bool running = true;
bool is_game_started = false;


struct Before {
	Before() {
		loadimage(&img_ice, _T("img/iced.png"));
	}
};
Before before;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	if (img == nullptr) cout << "no!!";
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

struct Vector {
	double x = 0, y = 0;
	double length = 0;
	Vector(double _x, double _y, bool normal = false) :x(_x), y(_y), length(sqrt(_x * _x + _y * _y)) {
		if (normal && length) {
			x /= length;
			y /= length;
		}
	}
	void normalize() {
		length = sqrt(x * x + y * y);
		if (length) {
			x /= length;
			y /= length;
		}
		length = sqrt(x * x + y * y);
	}
	void set_x(double _x) {
		x = _x;
		if (x && y) {
			if (abs(x) == 1) x *= 0.7;
			if (abs(y) == 1) y *= 0.7;
		}
		length = sqrt(x * x + y * y);
	}
	void set_y(double _y) {
		y = _y;
		if (x && y) {
			if (abs(x) == 1) x *= 0.7;
			if (abs(y) == 1) y *= 0.7;
		}
		length = sqrt(x * x + y * y);
	}
};

class Atlas {
public:
	Atlas(LPCTSTR path, int num) {
		size = num;
		TCHAR path_file[256];
		for (int i = 0; i < num; i++) {
			_stprintf_s(path_file, 256, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
		sketch_imgs = get_sketch_frame();
		another_side = get_right_frame();
		iced_imgs = get_iced_frame();
		atlases.push_back(this);
	}

	Atlas(int num) {
		size = num;
		for (int i = 0; i < num; i++) {
			IMAGE* frame = new IMAGE();
			frame_list.push_back(frame);
		}
		atlases.push_back(this);
	}
	
	~Atlas(){
		for (int i = 0; i < frame_list.size(); i++) delete frame_list[i]; 
	}

	Atlas* get_right_frame() {
		Atlas* right = new Atlas(size);
		for (int i = 0; i < size; i++) {
			int width = frame_list[i]->getwidth();
			int height = frame_list[i]->getheight();
			Resize(right->frame_list[i], width, height);
			DWORD* color_buffer_left_img = GetImageBuffer(frame_list[i]);
			DWORD* color_buffer_right_img = GetImageBuffer(right->frame_list[i]);
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int idx_left_img = y * width + x;
					int idx_right_img = y * width + width - x - 1;
					color_buffer_right_img[idx_right_img] = color_buffer_left_img[idx_left_img];
				}
			}
		}
		right->sketch_imgs = right->get_sketch_frame();
		right->iced_imgs = right->get_iced_frame();

		return right;
	}

	Atlas* get_sketch_frame() {
		Atlas* sketch = new Atlas(size);
		for (int i = 0; i < size; i++) {
			int width = frame_list[i]->getwidth();
			int height = frame_list[i]->getheight();
			Resize(sketch->frame_list[i], width, height);
			DWORD* color_buffer_raw_img = GetImageBuffer(frame_list[i]);
			DWORD* color_buffer_sketch_img = GetImageBuffer(sketch->frame_list[i]);
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int idx = y * width + x;
					if ((color_buffer_raw_img[idx] & 0xFF000000) >> 24) color_buffer_sketch_img[idx] = BGR(RGB(255, 255, 255)) | (((DWORD)(BYTE)(255)) << 24);
				}
			}
		}
		return sketch;
	}

	vector<Atlas*> get_iced_frame() {
		vector<Atlas*> ices;
		static const int THICKNESS = 5;
		static const float THRESHOLD = 0.84f;
		static const float RATIO = 0.25f;
		int width = frame_list[0]->getwidth();
		int height = frame_list[0]->getheight();
		int ct = height / THICKNESS;
		for (int i = 0; i < size; i++) {
			Atlas* ice = new Atlas(ct);
			int width = frame_list[i]->getwidth();
			int height = frame_list[i]->getheight();
			DWORD* color_buffer_raw_img = GetImageBuffer(frame_list[i]);
			DWORD* color_buffer_ice_img = GetImageBuffer(&img_ice);
			for (int j = 0; j < ct; j++) {
				int h = j * THICKNESS;
				Resize(ice->frame_list[j], width, height);
				DWORD* color_buffer_frame_img = GetImageBuffer(ice->frame_list[j]);
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						int idx = y * width + x;
						DWORD color_ice_img = color_buffer_ice_img[idx];
						DWORD color_frame_img = color_buffer_frame_img[idx];
						DWORD color_raw_img = color_buffer_raw_img[idx];
						if ((color_raw_img & 0xFF000000) >> 24) {
							color_frame_img = color_raw_img;
							BYTE r = (BYTE)(GetBValue(color_frame_img) * RATIO + GetBValue(color_ice_img) * (1 - RATIO));
							BYTE g = (BYTE)(GetGValue(color_frame_img) * RATIO + GetGValue(color_ice_img) * (1 - RATIO));
							BYTE b = (BYTE)(GetRValue(color_frame_img) * RATIO + GetRValue(color_ice_img) * (1 - RATIO));
							if ((y >= h && y <= h + THICKNESS) && ((r / 255.0f) * 0.2126f + (g / 255.0f) * 0.7152f + (b / 255.0f) * 0.0722f >= THRESHOLD)) {
								color_buffer_frame_img[idx] = BGR(RGB(255, 255, 255)) | (((DWORD)(BYTE)(255)) << 24);
								continue;
							}
							color_buffer_frame_img[idx] = BGR(RGB(r, g, b)) | (((DWORD)(BYTE)(255)) << 24);
						}
					}
				}
			}
			ices.push_back(ice);
		}
		return ices;
	}

public:
	static vector <Atlas*> atlases;
	vector<IMAGE*> frame_list;
	int size;
	Atlas* another_side = nullptr;
	Atlas* sketch_imgs = nullptr;
	vector<Atlas*> iced_imgs;
};
vector<Atlas*> Atlas::atlases;
Atlas* atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
Atlas* atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 6);

class Animation {
public:
	Animation(Atlas* atlas, int interval) {
		interval_ms = interval;
		anim_atlas = atlas;
		if (atlas->sketch_imgs) ske_anim = new Animation(atlas->sketch_imgs, interval);
		for (auto it : atlas->iced_imgs) {
			Animation* i_anim = new Animation(it, interval);
			ice_anim.push_back(i_anim);
		}
	}

	~Animation() = default;

	void Play(int x, int y, int delta) {
		timer += delta;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	}

	int When() {
		return idx_frame;
	}

public:
	Atlas* anim_atlas;
	Animation* ske_anim;
	vector<Animation*> ice_anim;
private:
	int timer = 0;
	int idx_frame = 0;
	int interval_ms = 0;
};

//class Living {
//public:
//	Animation* anim_left;
//	Animation* anim_right;
//};

class Player {
public:
	Player() {
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(atlas_player_left, 45);
		anim_right = new Animation(atlas_player_left->another_side, 45);
		act_vec = new Vector(0, 0);
		curr_anim = anim_left;
	}
	~Player() {
		delete anim_left;
		delete anim_right;
		delete act_vec;
	}

	void ProcessEvent(const ExMessage& msg) {
		//处理按键事件
		if (msg.message == WM_KEYDOWN) {
			switch (msg.vkcode) {
			case 0x57:
				is_w_pressed = true;
				act_vec->set_y(-1);
				break;
			case 0x53:
				is_s_pressed = true;
				act_vec->set_y(1);
				break;
			case 0x41:
				is_a_pressed = true;
				act_vec->set_x(-1);
				break;
			case 0x44:
				is_d_pressed = true;
				act_vec->set_x(1);
				break;
			case VK_ESCAPE:
				running = false;
			case 0x50:
				//is_iced = ice_time;
				attacked_counter = attacked_time;
			}
		}
		else if (msg.message == WM_KEYUP) {
			switch (msg.vkcode) {
			case 0x57:
				is_w_pressed = false;
				if (is_s_pressed) act_vec->set_y(1);
				else act_vec->set_y(0);
				if (is_a_pressed) act_vec->set_x(-1);
				if (is_d_pressed) act_vec->set_x(1);
				break;
			case 0x53:
				is_s_pressed = false;
				if (is_w_pressed) act_vec->set_y(-1);
				else act_vec->set_y(0);
				if (is_a_pressed) act_vec->set_x(-1);
				if (is_d_pressed) act_vec->set_x(1);
				break;
			case 0x41:
				is_a_pressed = false;
				if (is_d_pressed) act_vec->set_x(1);
				else act_vec->set_x(0);
				if (is_w_pressed) act_vec->set_y(-1);
				if (is_s_pressed) act_vec->set_y(1);
				break;
			case 0x44:
				is_d_pressed = false;
				if (is_a_pressed) act_vec->set_x(-1);
				else act_vec->set_x(0);
				if (is_w_pressed) act_vec->set_y(-1);
				if (is_s_pressed) act_vec->set_y(1);
				break;
			}
		}
	}

	void Move() {
		//玩家运动
		pos.x += (int)(act_vec->x * speed);
		pos.y += (int)(act_vec->y * speed);
		if (pos.x < 0) pos.x = 0;
		if (pos.y < 0) pos.y = 0;
		if (pos.x + PLAYER_WIDTH > WINDOW_WIDTH) pos.x = WINDOW_WIDTH - PLAYER_WIDTH;
		if (pos.y + PLAYER_HEIGHT > WINDOW_HEIGHT) pos.y = WINDOW_HEIGHT - PLAYER_HEIGHT;
		center.x = pos.x + PLAYER_WIDTH / 2;
		center.y = pos.y + PLAYER_HEIGHT / 2;
	};

	void Draw(int delta) {
		pos_shadow_x = pos.x + (PLAYER_WIDTH / 2 - SHADOW_WIDTH / 2);
		pos_shadow_y = pos.y + PLAYER_HEIGHT - 8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
		
		if (!attacked_counter && !is_iced) {
			if (act_vec->x < 0) {
				facing_left = true;
				curr_anim = anim_left;
			}
			else if (act_vec->x > 0) {
				facing_left = false;
				curr_anim = anim_right;
			}
			else curr_anim = facing_left ? anim_left : anim_right;
		}
		if (attacked_counter==attacked_time) curr_anim = curr_anim->ske_anim;
		if (attacked_counter) attacked_counter--;
		if (is_iced == ice_time) curr_anim = curr_anim->ice_anim[curr_anim->When()];
		if (is_iced) is_iced--;

		curr_anim->Play(pos.x, pos.y, delta);
	}

	const POINT& GetPosition() const {
		return pos;
	}

	const POINT& GetSize() const {
		return size;
	}

	const POINT& GetCenter() const {
		return center;
	}

private:
	static const int PLAYER_ANIM_NUM = 6;
	static const int PLAYER_WIDTH = 80, PLAYER_HEIGHT = 80;
	POINT size = { PLAYER_WIDTH,PLAYER_HEIGHT };
	static const int SHADOW_WIDTH = 32;
	IMAGE img_player_left[PLAYER_ANIM_NUM];
	IMAGE img_player_right[PLAYER_ANIM_NUM];
	IMAGE img_shadow;
	POINT pos = { 500,500 };
	POINT center = { pos.x + PLAYER_WIDTH / 2,pos.y + PLAYER_HEIGHT / 2 };
	int pos_shadow_x, pos_shadow_y;
	int speed = 8;
	bool is_w_pressed = false, is_s_pressed = false, is_a_pressed = false, is_d_pressed = false;
	Vector* act_vec;
	Animation* anim_left;
	Animation* anim_right;
	Animation* curr_anim;
	bool facing_left = false;
	int attacked_time = fps / 8;
	int attacked_counter = attacked_time;
	int ice_time = fps;
	int is_iced = 0;
};
void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分：%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

class Bullet {
public:
	POINT pos = { 0,0 };

	Bullet() = default;
	~Bullet() = default;

	void Draw() const {
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(pos.x, pos.y, radius);
	}
private:
	int radius = 10;
};
double radial_speed = 0.0045, tangent_speed = 0.0055;//径向与切向速度
void UpdateBullets(vector<Bullet>& bullet_list, const Player& player) {
	double radian_interval = 2 * 3.1415 / bullet_list.size();
	POINT player_pos = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * radial_speed);//径向半径
	double radian;//当前子弹所在弧度值
	for (int i = 0; i < bullet_list.size(); i++) {
		radian = GetTickCount() * tangent_speed + radian_interval * i;
		bullet_list[i].pos.x = player.GetCenter().x + (int)(radius * sin(radian));
		bullet_list[i].pos.y = player.GetCenter().y + (int)(radius * cos(radian));
	}
}

class Enemy {
public:
	Enemy() {
		anim_left = new Animation(atlas_enemy_left, 45);
		anim_right = new Animation(atlas_enemy_left->another_side, 45);
		act_vec = new Vector(0, 0);

		enum class SpawnEdge {
			Up=0,
			Down,
			Left,
			Right
		};
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge) {
		case SpawnEdge::Up:
			pos.x = rand() % WINDOW_WIDTH;
			pos.y = -FRAME_HEIGHT;
			break;
		case SpawnEdge::Down:
			pos.x = rand() % WINDOW_WIDTH;
			pos.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left:
			pos.x = -FRAME_WIDTH;
			pos.y = rand() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right:
			pos.x = WINDOW_WIDTH;
			pos.y = rand() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}

	~Enemy() {
		delete anim_left;
		delete anim_right;
		delete act_vec;
	}

	bool CheckBulletCollision(const Bullet& bullet) {
		is_overlap_x = bullet.pos.x >= pos.x && bullet.pos.x <= pos.x + FRAME_WIDTH;
		is_overlap_y = bullet.pos.y >= pos.y && bullet.pos.y <= pos.y + FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}

	bool CheckPlayerCollision(const Player& player) {
		check_pos = { pos.x + FRAME_WIDTH / 2,pos.y + FRAME_HEIGHT / 2 };
		return (check_pos.x >= player.GetPosition().x && check_pos.x <= player.GetPosition().x + player.GetSize().x) && (check_pos.y >= player.GetPosition().y && check_pos.y <= player.GetPosition().y + player.GetSize().y);
	}

	void Move(const Player& player) {
		const POINT& player_pos = player.GetPosition();
		act_vec->set_x((player_pos.x - pos.x > 0) ? 1 : -1);
		act_vec->set_y((player_pos.y - pos.y > 0) ? 1 : -1);

		pos.x += (int)(act_vec->x * speed);
		pos.y += (int)(act_vec->y * speed);
	}

	void Draw(int delta, IMAGE shadow) {
		pos_shadow_x = pos.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		pos_shadow_y = pos.y + FRAME_HEIGHT - 30;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &shadow);

		if (act_vec->x < 0) facing_left = true;
		else if (act_vec->x > 0) facing_left = false;
		if (facing_left) anim_left->Play(pos.x, pos.y, delta);
		else anim_right->Play(pos.x, pos.y, delta);
	}

	void Hurt() {
		alive = false;
	}

	bool CheckAlive() {
		return alive;
	}

private:
	int speed = 4;
	const int FRAME_WIDTH = 80, FRAME_HEIGHT = 80;
	const int SHADOW_WIDTH = 40;
	Animation* anim_left;
	Animation* anim_right;
	POINT pos = { 0,0 };
	bool facing_left = false;
	Vector* act_vec;
	int pos_shadow_x, pos_shadow_y;
	bool is_overlap_x, is_overlap_y;
	POINT check_pos;
	bool alive = true;
};
int generate_interval = 100;
void TryGenerateEnemy(vector<Enemy*>& enemy_list) {
	static int counter = 0;
	if ((++counter) % generate_interval == 0) enemy_list.push_back(new Enemy());
}

class Button {
public:
	Button(RECT rect,LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed) {
		region = rect;
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
	}

	~Button() = default;

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message){
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y)) status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y)) status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y)) status = Status::Pushed;
			else status = Status::Idle;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed && CheckCursorHit(msg.x, msg.y)) OnClick();
		default:
			break;
		}
	}

	void Draw() {
		switch (status) {
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}

private:
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}

protected:
	virtual void OnClick() = 0;

private:
	RECT region;
	IMAGE img_idle, img_hovered, img_pushed;
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed
	};
	Status status = Status::Idle;
};

class StartGameButton :public Button {
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed) :Button(rect, path_img_idle, path_img_hovered, path_img_pushed){}
	~StartGameButton() = default;
protected:
	void OnClick() {
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

class QuitGameButton :public Button {
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed) :Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default;
protected:
	void OnClick() {
		running = true;
	}
};

int main() {
	//初始化窗口
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);

	//初始化变量
	DWORD start_time;
	DWORD end_time;
	ExMessage msg;
	Player player;
	vector<Enemy*> enemy_list;
	Enemy* checked_enemy;
	vector<Bullet> bullet_list(3, Bullet());
	int score = 0;

	RECT region_btn_start_game, region_btn_quit_game;
	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = 430 + BUTTON_HEIGHT;
	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;
	StartGameButton btn_start_game(region_btn_start_game, _T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game(region_btn_quit_game, _T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	IMAGE enemy_shadow;
	loadimage(&enemy_shadow, _T("img/shadow_enemy.png"));
	loadimage(&img_background, _T("img/background.png"));
	loadimage(&img_menu, _T("img/menu.png"));

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav type mpegvideo alias hit"), NULL, 0, NULL);
	
	//开启双缓冲
	BeginBatchDraw();
	//主循环
	while (running) {
		//开始计时
		start_time = GetTickCount();

		//捕获事件
		while (peekmessage(&msg)) {
			if (is_game_started) player.ProcessEvent(msg);
			else {
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}

		if (is_game_started) {
			//处理事件
			player.Move();
			UpdateBullets(bullet_list, player);
			TryGenerateEnemy(enemy_list);

			for (Enemy* enemy : enemy_list) {
				enemy->Move(player);
				//检测碰撞
				if (enemy->CheckPlayerCollision(player)) {
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分：%d ！\n扣“1”观看战败CG"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
				for (const Bullet& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						enemy->Hurt();
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						/*mciSendString(_T(""), NULL, 0, NULL);*/
						break;
					}
				}
			}
			//移除死亡敌人
			for (int i = 0; i < enemy_list.size(); i++) {
				checked_enemy = enemy_list[i];
				if (!checked_enemy->CheckAlive()) {
					score++;
					swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete checked_enemy;
					if (i > 0) i--;
				}
			}
		}

		//清空窗口
		cleardevice();

		//开始绘图
		if (is_game_started) {
			putimage(0, 0, &img_background);
			player.Draw(15);
			for (Enemy* enemy : enemy_list) enemy->Draw(15, enemy_shadow);
			for (Bullet bullet : bullet_list) bullet.Draw();
			DrawPlayerScore(score);
		}
		else {
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}

		//刷新双缓冲
		FlushBatchDraw();

		//结束计时
		end_time = GetTickCount();
		//计算帧耗时并睡眠降帧，优化性能
		Sleep(fr_intv > (end_time - start_time) ? fr_intv - (end_time - start_time) : fr_intv);
	}
	for (auto it : Atlas::atlases) {
		delete it;
	}
	//结束双缓冲
	EndBatchDraw();

	return 0;
}