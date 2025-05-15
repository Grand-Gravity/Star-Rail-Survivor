#include<graphics.h>
#include<random>
#include<string>
#include<vector>
#include<list>
using namespace std;
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"MSIMG32.LIB")
#define pi 3.14159265359
#include<iostream>

//此处可调帧率vvv
const int fps = 60, fr_intv = 1000 / fps;
const int WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720;
const int BUTTON_WIDTH = 192, BUTTON_HEIGHT = 75;
mt19937 engine((unsigned int)time(NULL));
int idx_current_anim = 0;
IMAGE img_background, img_menu, img_ice, img_bkg2;
bool running = true;
bool is_game_started = false;
int score = 0;
int& difficulty = score;
int experience = 0, span = 3;
int level = 0;
int enemy_hp = 100, enemy_atk = 100, enemy_speed = 4;
bool option = false;
//RECT UI_rect = { 20,85,450,110 };


struct Before {
	Before() {
		loadimage(&img_ice, _T("imgs/UI/iced.png"));
	}
};
Before before;

inline int putimage_alpha(int x, int y, IMAGE* img, int w = 0, int h = 0) {
	if (!w && !h) {
		w = img->getwidth();
		h = img->getheight();
	}
	if (img == nullptr) cout << "no!!";
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, img->getwidth(), img->getheight(), { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
	return w;
}

IMAGE* rotateimage_alpha(IMAGE* pImg, double radian, COLORREF bkcolor = BLACK)
{
	radian = -radian;														// 由于 y 轴翻转，旋转角度需要变负
	float fSin = (float)sin(radian), fCos = (float)cos(radian);				// 存储三角函数值
	float fNSin = (float)sin(-radian), fNCos = (float)cos(-radian);
	int left = 0, top = 0, right = 0, bottom = 0;							// 旋转后图像顶点
	int w = pImg->getwidth(), h = pImg->getheight();
	DWORD* pBuf = GetImageBuffer(pImg);
	POINT points[4] = { {0, 0}, {w, 0}, {0, h}, {w, h} };					// 存储图像顶点
	for (int j = 0; j < 4; j++)												// 旋转图像顶点，搜索旋转后的图像边界
	{
		points[j] = {
			(int)(points[j].x * fCos - points[j].y * fSin),
			(int)(points[j].x * fSin + points[j].y * fCos)
		};
		if (points[j].x < points[left].x)	left = j;
		if (points[j].y > points[top].y)	top = j;
		if (points[j].x > points[right].x)	right = j;
		if (points[j].y < points[bottom].y)	bottom = j;
	}
	int nw = points[right].x - points[left].x;								// 旋转后的图像尺寸
	int nh = points[top].y - points[bottom].y;
	int nSize = nw * nh;
	int offset_x = points[left].x < 0 ? points[left].x : 0;					// 旋转后图像超出第一象限的位移（据此调整图像位置）
	int offset_y = points[bottom].y < 0 ? points[bottom].y : 0;
	IMAGE* img=new IMAGE(nw, nh);
	DWORD* pNewBuf = GetImageBuffer(img);
	if (bkcolor != BLACK)													// 设置图像背景色
		for (int i = 0; i < nSize; i++)
			pNewBuf[i] = BGR(bkcolor);
	for (int i = offset_x, ni = 0; ni < nw; i++, ni++)						// i 用于映射原图像坐标，ni 用于定位旋转后图像坐标
	{
		for (int j = offset_y, nj = 0; nj < nh; j++, nj++)
		{
			int nx = (int)(i * fNCos - j * fNSin);							// 从旋转后的图像坐标向原图像坐标映射
			int ny = (int)(i * fNSin + j * fNCos);
			if (nx >= 0 && nx < w && ny >= 0 && ny < h)						// 若目标映射在原图像范围内，则拷贝色值
				pNewBuf[nj * nw + ni] = pBuf[ny * w + nx];
		}
	}
	return img;
}

bool point_rect_collision(POINT p, POINT rp, int w, int h) {
	return p.x >= rp.x && p.x <= rp.x + w && p.y >= rp.y && p.y <= rp.y + h;
}

bool rect_rect_collision(POINT rp1_0, int w1, int h1, POINT rp2, int w2, int h2) {
	POINT rp1_1 = { rp1_0.x + w1,rp1_0.y }, rp1_2 = { rp1_0.x,rp1_0.y + h1 }, rp1_3 = { rp1_0.x + w1,rp1_0.y + h1 };
	return point_rect_collision(rp1_0, rp2, w2, h2) || point_rect_collision(rp1_1, rp2, w2, h2) || point_rect_collision(rp1_2, rp2, w2, h2) || point_rect_collision(rp1_3, rp2, w2, h2);
}

void GameOver() {
	static TCHAR text[128];
	_stprintf_s(text, _T("最终得分：%d !"), score);
	MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
	running = false;
}

struct Vector {
	double x = 0, y = 0;
	double length = 0;
	Vector(double _x, double _y, bool normal = false) :x(_x), y(_y), length(sqrt(_x* _x + _y * _y)) {
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
	Atlas(LPCTSTR path, int num, bool isliving = false) {
		size = num;
		living = isliving;
		TCHAR path_file[256];
		for (int i = 0; i < num; i++) {
			_stprintf_s(path_file, 256, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
		w = frame_list[0]->getwidth();
		h = frame_list[0]->getheight();
		another_side = get_right_frame();
		if (living) {
			sketch_imgs = get_sketch_frame();
			iced_imgs = get_iced_frame();
		}
		else {
			up_side = get_up_frame();
			down_side = get_down_frame();
		}
		all_atlases.push_back(this);
	}

	Atlas(int num) {
		size = num;
		for (int i = 0; i < num; i++) {
			IMAGE* frame = new IMAGE();
			frame_list.push_back(frame);
		}
		all_atlases.push_back(this);
	}

	~Atlas() {
		for (auto i : frame_list) delete i;
		delete another_side;
		delete sketch_imgs;
		for (auto i : iced_imgs) delete i;
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
		right->w = frame_list[0]->getwidth();
		right->h = frame_list[0]->getheight();
		if (living) {
			right->sketch_imgs = right->get_sketch_frame();
			right->iced_imgs = right->get_iced_frame();
		}
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
		sketch->w = frame_list[0]->getwidth();
		sketch->h = frame_list[0]->getheight();
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
			ice->w = frame_list[0]->getwidth();
			ice->h = frame_list[0]->getheight();
			ices.push_back(ice);
		}
		return ices;
	}

	Atlas* get_up_frame() {
		Atlas* up = new Atlas(size);
		for (int i = 0; i < size; i++) {
			up->frame_list[i] = rotateimage_alpha(frame_list[i], pi / 2);
		}
		up->w = up->frame_list[0]->getwidth();
		up->h = up->frame_list[0]->getheight();
		return up;
	}

	Atlas* get_down_frame() {
		Atlas* down = new Atlas(size);
		for (int i = 0; i < size; i++) {
			down->frame_list[i] = rotateimage_alpha(frame_list[i], -pi / 2);
		}
		down->w = down->frame_list[0]->getwidth();
		down->h = down->frame_list[0]->getheight();
		return down;
	}

public:
	static vector <Atlas*> all_atlases;
	vector<IMAGE*> frame_list;
	int size, w, h;
	bool living = false;
	Atlas* another_side = nullptr;
	Atlas* sketch_imgs = nullptr;
	vector<Atlas*> iced_imgs;
	Atlas* up_side = nullptr;
	Atlas* down_side = nullptr;
};
vector<Atlas*> Atlas::all_atlases;
Atlas* atlas_player_left = new Atlas(_T("imgs/Player/idle/idle_%d.png"), 5, true);
Atlas* atlas_player_back = new Atlas(_T("imgs/Player/back/back_%d.png"), 1, true);
Atlas* atlas_player_walk = new Atlas(_T("imgs/Player/walk/walk_%d.png"), 6, true);
Atlas* atlas_enemy_black = new Atlas(_T("imgs/Enemy/Black/run_%d.png"), 6, true);
Atlas* atlas_enemy_brown = new Atlas(_T("imgs/Enemy/Brown/run_%d.png"), 6, true);
Atlas* atlas_enemy_gray = new Atlas(_T("imgs/Enemy/Gray/run_%d.png"), 6, true);
Atlas* atlas_bullet = new Atlas(_T("imgs/Bullet/bullet_%d.png"), 10);

class Animation {
public:
	Animation(Atlas* atlas, int interval, int rt = -1, bool no_more = false) {
		interval_ms = interval;
		anim_atlas = atlas;
		re_times = rt;
		w = anim_atlas->frame_list[0]->getwidth();
		h = anim_atlas->frame_list[0]->getheight();
		if (re_times == -1) {
			if (atlas->another_side) ano_anim = new Animation(atlas->another_side, interval);
			if (atlas->sketch_imgs) ske_anim = new Animation(atlas->sketch_imgs, interval);
			for (auto it : atlas->iced_imgs) {
				Animation* i_anim = new Animation(it, interval);
				ice_anim.push_back(i_anim);
			}
			all_animations.push_back(this);
		}
		else {
			if (atlas->another_side) ano_anim = new Animation(atlas->another_side, interval, rt);
			if (atlas->up_side) up_anim = new Animation(atlas->up_side, interval);
			if (atlas->down_side) down_anim = new Animation(atlas->down_side, interval);
		}
	}

	~Animation() {
		delete ano_anim;
		delete ske_anim;
		for (auto i : ice_anim) delete i;
	}

	bool Play(int x, int y, int delta, int ex = 0, int ey = 0) {
		timer += delta;
		if (timer >= interval_ms) {
			if (re_times != -1 && idx_frame == anim_atlas->size - 1) re_times--;
			idx_frame = (idx_frame + 1) % anim_atlas->size;
			timer = 0;
		}
		if (re_times != 0) {
			if (ex && ey) putimage_alpha(x, y, anim_atlas->frame_list[idx_frame], ex, ey);
			else putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
		}
		else {
			delete this;
			return false;
		}
		return true;
	}

	int When() {
		return idx_frame;
	}

public:
	static vector<Animation*> all_animations;
	Atlas* anim_atlas;
	int w = 0, h = 0;
	int re_times;
	Animation* ano_anim = nullptr;
	Animation* ske_anim = nullptr;
	vector<Animation*> ice_anim;
	Animation* up_anim = nullptr;
	Animation* down_anim = nullptr;
private:
	int timer = 0;
	int idx_frame = 0;
	int interval_ms = 0;
};
vector<Animation*> Animation::all_animations;

class Status {
public:
	bool allow(int ts) {
		vector<int> r;
		switch (s) {
		case Status::Attacked:
			r = r_Attacked;
			break;
		case Status::Iced:
			r = r_Iced;
			break;
		}
		for (int i : r) if (i == ts) return false;
		return true;
	}

	void to(int ts, bool compulsory = false) {
		if (this->allow(ts) || compulsory) s = ts;
	}
public:
	static const int Idle = 0, 
		Walking = 1, 
		Attacking = 2, 
		Attacked = 3, 
		Iced = 4;
	int s = Status::Idle;

	vector<int> r_Attacked = { 3 };//受击时不能连续受击
	vector<int> r_Iced = { 0,1,2,3,4 };//被冰冻时无法切换任何状态
};

class Living {
public:
	virtual bool Hurt(int damage) = 0;
	const POINT& GetPosition() const {
		return pos;
	}
public:
	POINT pos = { 0,0 };
	Vector* act_vec;
	int width, height;
	int atk;
	int hurt = 0;
	int ice_time = 0, is_iced = ice_time;
	Status status;
protected:
	int attacked_time;
	int attacked_counter;
};

class Player;
class Effect {
public:
	Effect(Living* o, Atlas* at, int re_times, int interval, int w = 0, int h = 0) {
		owner = o;
		atk = owner->atk;
		atlas = at;
		anim = new Animation(atlas, interval, re_times);
		ano_anim = anim->ano_anim;
		if (!w && !h) {
			width = atlas->w;
			height = atlas->h;
		}
		else {
			width = w;
			height = h;
		}
	}
	virtual ~Effect() {};
	POINT GetPosition() {
		return pos;
	}
	virtual void Update(Player* p) {};
	virtual void Affect(Living* tag, int type = 0, int t2 = 0) = 0;
	virtual bool Play(int delta) = 0;
protected:
	Atlas* atlas;
	Animation* anim;
	Animation* ano_anim;
	int atk = 0;
public:
	Living* owner;
	POINT pos = { 0,0 };
	int width = 0, height = 0;
	POINT center = { pos.x + width / 2,pos.y + height / 2 };
	bool facing_left = true, facing_up = false;
};

class Hit :public Effect {
public:
	using Effect::Effect;
	~Hit() {}
	void Update(Player* p);
	void FitCenter(Player* p);
	bool Play(int delta) {
		if (owner->act_vec->x < 0) {
			pos.x -= owner->width / 2 + width / 2;
			return anim->Play(pos.x, pos.y, delta, width, height);
		}
		else if (owner->act_vec->x > 0) {
			pos.x += owner->width / 2 + width / 2;
			return anim->ano_anim->Play(pos.x, pos.y, delta, width, height);
		}
		else {
			if (owner->act_vec->y < 0) {
				pos.y -= owner->height / 2 + height;
				return anim->Play(pos.x, pos.y, delta, width, height);
			}
			else if (owner->act_vec->y > 0) {
				pos.y += owner->height / 2 + height;
				return anim->ano_anim->Play(pos.x, pos.y, delta, width, height);
			}
			else {
				if (facing_left) {
					pos.x -= owner->width / 2 + width / 2;
					return anim->Play(pos.x, pos.y, delta, width, height);
				}
				else {
					pos.x += owner->width / 2 + width / 2;
					return anim->ano_anim->Play(pos.x, pos.y, delta, width, height);
				}
			}
		}
	}
	void Affect(Living* tag, int type = 0, int t2 = 0);
public:
	static Atlas* at;
};
Atlas* Hit::at=new Atlas(_T("imgs/Effects/hit/hit_%d.png"), 3);

class Bullet {
public:
	Bullet() {
		anim = new Animation(atlas_bullet, 120);
	}
	~Bullet() {
		delete anim;
	}

	void Draw(Player* p);

public:
	POINT pos = { 0,0 };
	Animation* anim;
	int radius = 10;
};
double radial_speed = 0.0045, tangent_speed = 0.0055;//径向与切向速度
vector<Bullet*> bullet_list;
void Hit::Affect(Living* tag, int type, int t2) {
	tag->ice_time = t2 * fps / 4;
	tag->is_iced = tag->ice_time;
	if (tag->Hurt(atk)) {
		for (int i = 0; i < type; i++) {
			bullet_list.push_back(new Bullet());
		}
	}
}

class Player :public Living {
public:
	Player() {
		attacked_time = fps / 6;
		attacked_counter = 0;
		loadimage(&img_shadow, _T("imgs/Player/shadow_player.png"));
		loadimage(&hp_frame, _T("imgs/UI/hpframe.png"));
		loadimage(&hp_bar, _T("imgs/UI/hpbar.png"));
		anim_idle = new Animation(atlas_player_left, 120);
		anim_back = new Animation(atlas_player_back, 120);
		anim_walk = new Animation(atlas_player_walk, 720 / speed);
		act_vec = new Vector(0, 0);
		curr_anim = anim_idle;
	}
	~Player() {
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
			case 0x4A:
				if (attack_counter == 0) {
					attack_counter = attack_cool;
					mciSendString(_T("play wave from 0"), NULL, 0, NULL);
					Attack();
				}
				break;
			case VK_ESCAPE:
				GameOver();
				break;
			/*case 0x49:
				bullet_list.push_back(new Bullet());
				break;
			case 0x4F:
				if (!bullet_list.empty()) {
					delete bullet_list.back();
					bullet_list.pop_back();
				}
				break;
			case 0x50:
				option = true;
				break;
			case 0x4C:
				is_iced = ice_time;
				break;*/
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

	const POINT& GetPosition() const {
		return pos;
	}

	const POINT& GetCenter() const {
		return center;
	}

	Animation* GetAnim() {
		return anim_idle;
	}

	const bool Attackable() const {
		return attacked_counter == 0;
	}

	void EffectEvents() {
		for (auto e = effects.begin(); e != effects.end();) {
			(*e)->Update(this);
			if (!((*e)->Play(40))) {
				delete (*e);
				e = effects.erase(e);
			}
			else e++;
		}
	}

	void Events() {
		//状态处理
		if (status.allow(Status::Idle) || status.allow(Status::Walking)/*attacked_counter <= 1 && !is_iced*/) {
			if (!act_vec->x && !act_vec->y) {
				status.to(Status::Idle);
				curr_anim = facing_up ? anim_back : anim_idle;
			}
			else {
				status.to(Status::Walking);
				if (act_vec->y) {
					if (facing_up) curr_anim = anim_back;
					else curr_anim = anim_idle;
				}
				if (act_vec->x) {
					if (facing_left) curr_anim = anim_walk;
					else curr_anim = anim_walk->ano_anim;
				}
			}
		}

		if (attacked_counter > 0/*== attacked_time*/) {
			status.to(Status::Attacked);
			curr_anim = curr_anim->ske_anim;
			attacked_counter--;
		}
		if (is_iced == ice_time) {
			status.to(Status::Iced);
			curr_anim = curr_anim->ice_anim[curr_anim->When()];
		}
		if (is_iced > 0) {
			is_iced--;
			if (is_iced == 0) status.to(Status::Idle, true);
		}

		if (attack_counter != 0) {
			status.to(Status::Attacking);
			attack_counter--;
		}
		if (hp > hp_max) hp = hp_max;
	}

	void Move() {
		//玩家运动
		pos.x += (int)(act_vec->x * speed);
		pos.y += (int)(act_vec->y * speed);
		if (pos.x < 0) pos.x = 0;
		if (pos.y < 0) pos.y = 0;
		if (pos.x + width > WINDOW_WIDTH) pos.x = WINDOW_WIDTH - width;
		if (pos.y + PLAYER_HEIGHT > WINDOW_HEIGHT) pos.y = WINDOW_HEIGHT - PLAYER_HEIGHT;

		center.x = pos.x + width / 2;
		center.y = pos.y + PLAYER_HEIGHT / 2;
		if (act_vec->x < 0) facing_left = true;
		else if (act_vec->x > 0) facing_left = false;
		if (act_vec->y < 0) facing_up = true;
		else if (act_vec->y > 0) facing_up = false;
		if (act_vec->x) facing_up = false;
	};

	void Draw(int delta) {
		width = curr_anim->w * PLAYER_HEIGHT / curr_anim->h;
		pos_shadow_x = pos.x + width / 2 - shadow_width / 2;
		pos_shadow_y = pos.y + PLAYER_HEIGHT - 8;
		shadow_width = width * 0.8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow, shadow_width, 20);
		curr_anim->Play(pos.x, pos.y, delta, width, PLAYER_HEIGHT);
		//显示碰撞箱及中心
		/*fillcircle(center.x, center.y, 5);
		rectangle(pos.x, pos.y, pos.x + width, pos.y + PLAYER_HEIGHT);*/
		//特效处理
		EffectEvents();
		//受伤数字显示
		if (attacked_counter > 0) {
			static TCHAR text[64];
			_stprintf_s(text, _T("-%d"), hurt);

			//setbkmode(TRANSPARENT);
			settextstyle(max((double)(hurt / 100.0), 1) * 20, 0, _T("IPix"));
			settextcolor(RGB(255, 51, 51));
			outtextxy(pos.x, pos.y - 30, text);
		}
	}

	bool Hurt(int damage) {
		mciSendString(_T("play hurt from 0"), NULL, 0, NULL);
		hp -= damage;
		hurt = damage;
		attacked_counter = attacked_time;
		return false;
	}

	void Attack() {
		Hit* hit = new Hit(this, Hit::at, 1, 120, atk_width, atk_height);
		effects.push_back(hit);
	}

public:
	static const int PLAYER_WIDTH = 80, PLAYER_HEIGHT = 155;
	int width = PLAYER_WIDTH, height = PLAYER_HEIGHT;
	bool facing_left = true, facing_up = false;
	int hp_max = 500, hp = hp_max;
	int speed_max = 8, speed = speed_max;
	double force = 30;//击退强度
	int hp_steal = 0;
	int atk = 50;
	int attack_cool = fps / 4, attack_counter = 0;
	int atk_width = 120, atk_height = 120;
	int make_bullet = 0;
	int make_ice = 0;
	list<Effect*> effects;
	IMAGE hp_frame, hp_bar;
private:
	static const int PLAYER_ANIM_NUM = 5;
	POINT size = { PLAYER_WIDTH,PLAYER_HEIGHT };
	int shadow_width = 64;
	IMAGE img_shadow;
	POINT pos = { WINDOW_WIDTH / 2 - width / 2,WINDOW_HEIGHT / 2 - height / 2 };
	POINT center = { pos.x + PLAYER_WIDTH / 2,pos.y + PLAYER_HEIGHT / 2 };
	int pos_shadow_x, pos_shadow_y;
	bool is_w_pressed = false, is_s_pressed = false, is_a_pressed = false, is_d_pressed = false;

	Animation* curr_anim;
	Animation* anim_idle;
	Animation* anim_back;
	Animation* anim_walk;
	int ice_time = fps;
	int is_iced = 0;
};
void DrawPlayerUI(int score,Player* p) {
	static TCHAR text[64];
	_stprintf_s(text, _T("生命：%d/%d   分数：%d   经验：%d/%d   等级：%d"), p->hp, p->hp_max, score, experience, (span + level), level);

	setbkmode(TRANSPARENT);
	settextstyle(25, 0, _T("IPix"));
	settextcolor(RGB(255, 255, 255));
	outtextxy(20, 85, text);
	//drawtext(text, &UI_rect, DT_WORDBREAK | DT_LEFT | DT_EDITCONTROL);
	putimage_alpha(35, 37, &p->hp_bar, (int)(450 * ((double)(p->hp) / (double)(p->hp_max))), 20);
	putimage_alpha(20, 20, &p->hp_frame, 480, 55);
}
void UpdateBullets(vector<Bullet*>& bullet_list, const Player& player) {
	double radian_interval = 2 * 3.1415 / bullet_list.size();
	double radius = 100 + 36 * sin(GetTickCount() * radial_speed);//径向半径
	double radian;//当前子弹所在弧度值
	for (int i = 0; i < bullet_list.size(); i++) {
		radian = GetTickCount() * tangent_speed + radian_interval * i;
		bullet_list[i]->pos.x = player.GetCenter().x + (int)(radius * sin(radian));
		bullet_list[i]->pos.y = player.GetCenter().y + (int)(radius * cos(radian));
	}
}
void Bullet::Draw(Player* p){
	/*setlinecolor(RGB(255, 155, 50));
	setfillcolor(RGB(200, 75, 10));
	fillcircle(pos.x, pos.y, radius);*/
	if (p->facing_left) anim->ano_anim->Play(pos.x - 32, pos.y - 32, 30, 64, 64);
	else anim->Play(pos.x - 32, pos.y - 32, 30, 64, 64);
}

void Hit::Update(Player* p) {
	FitCenter(p);
	facing_left = p->facing_left;
	facing_up = p->facing_up;
	atk = p->atk;
}
void Hit::FitCenter(Player* p) {
	pos = p->GetPosition();
	pos.x += p->width / 2 - width / 2;
	pos.y += p->height / 2 - height / 2;
}

class Enemy :public Living {
public:
	Enemy() {
		hp_max = enemy_hp;
		atk = enemy_atk;
		speed_max = enemy_speed;
		switch (engine() % 3) {
		case 0:
			//黑狗跑得慢，伤害高
			anim_left = new Animation(atlas_enemy_black, 50);
			speed_max *= 0.75;
			atk *= 1.5;
			break;
		case 1:
			//棕狗均衡,血厚
			hp_max *= 1.5;
			anim_left = new Animation(atlas_enemy_brown, 45);
			break;
		case 2:
			//灰狗跑得快，伤害略低
			anim_left = new Animation(atlas_enemy_gray, 30);
			speed_max *= 1.5;
			atk *= 0.5;
			break;
		}
		curr_anim = anim_left;
		act_vec = new Vector(0, 0);
		pas_vec = new Vector(0, 0);
		hp = hp_max;
		speed = speed_max;
		attacked_time = fps / 6;
		attacked_counter = 0;
		ice_time = 0, is_iced = ice_time;
		width = FRAME_WIDTH;
		height = FRAME_HEIGHT;
		loadimage(&hp_bk, _T("imgs/UI/hpbk.png"));
		loadimage(&hp_bar, _T("imgs/UI/hpbar.png"));

		enum class SpawnEdge {
			Up = 0,
			Down,
			Left,
			Right
		};
		SpawnEdge edge = (SpawnEdge)(engine() % 4);
		switch (edge) {
		case SpawnEdge::Up:
			pos.x = engine() % WINDOW_WIDTH;
			pos.y = -FRAME_HEIGHT;
			break;
		case SpawnEdge::Down:
			pos.x = engine() % WINDOW_WIDTH;
			pos.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left:
			pos.x = -FRAME_WIDTH;
			pos.y = engine() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right:
			pos.x = WINDOW_WIDTH;
			pos.y = engine() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}

	~Enemy() {
		delete anim_left;
		delete act_vec;
	}

	bool CheckBulletCollision(const Bullet* bullet) {
		return point_rect_collision(bullet->pos, pos, FRAME_WIDTH, FRAME_HEIGHT);
	}

	bool CheckPlayerCollision(const Player& player) {
		return point_rect_collision(player.GetCenter(), pos, FRAME_WIDTH, FRAME_HEIGHT);
	}

	void Move(const Player& player) {
		POINT player_pos = { player.GetCenter().x - (width / 2),player.GetCenter().y - (height / 2) };
		act_vec->set_x((player_pos.x - pos.x > 0) ? 1 : -1);
		act_vec->set_y((player_pos.y - pos.y > 0) ? 1 : -1);
		if (is_iced) {
			speed = 0;
			is_iced--;
		}
		else speed = speed_max;
		pos.x += (int)(act_vec->x * speed) - (int)(pas_vec->x * player.force);
		pos.y += (int)(act_vec->y * speed) - (int)(pas_vec->y * player.force);
		center = { pos.x + FRAME_WIDTH / 2,pos.y + FRAME_HEIGHT / 2 };
		if (pas_vec->x > 0) pas_vec->x -= 0.1;
		else if (pas_vec->x < 0) pas_vec->x += 0.1;
		if (abs(pas_vec->x) <= 0.1) pas_vec->x = 0;
		if (pas_vec->y > 0) pas_vec->y -= 0.1;
		else if (pas_vec->y < 0) pas_vec->y += 0.1;
		if (abs(pas_vec->y) <= 0.1) pas_vec->y = 0;
	}

	void Draw(int delta, IMAGE shadow) {
		pos_shadow_x = pos.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		pos_shadow_y = pos.y + FRAME_HEIGHT - 20;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &shadow);

		if (act_vec->x < 0) facing_left = true;
		else if (act_vec->x > 0) facing_left = false;
		if (attacked_counter > 0/*== attacked_time*/) {
			curr_anim = anim_left->ske_anim;
			attacked_counter--;
		}
		else {
			if (facing_left) curr_anim = anim_left;
			else curr_anim = anim_left->ano_anim;
		}
		if (is_iced) {
			if (facing_left) curr_anim = anim_left->ice_anim[curr_anim->When()];
			else curr_anim = anim_left->ano_anim->ice_anim[curr_anim->When()];
		}
		curr_anim->Play(pos.x, pos.y, delta, FRAME_WIDTH, FRAME_HEIGHT);
		putimage_alpha(pos.x, pos.y - 10, &hp_bk, width, 10);
		putimage_alpha(pos.x, pos.y - 10, &hp_bar, (int)(width * ((double)(hp) / (double)(hp_max))), 10);
		//受伤数字显示
		if (hurt_counter > 0 && hurt != 0) {
			text[0] = '\0';
			_stprintf_s(text, _T("-%d"), hurt);

			settextstyle(max((double)(hurt / (double)(hp_max)), 1) * 20, 0, _T("IPix"));
			settextcolor(RGB(255, 102, 102));
			outtextxy(pos.x, pos.y - 35, text);
			hurt_counter--;
		}
	}

	void Events(Player* p) {
		for (auto e : p->effects) {
			if (rect_rect_collision(pos, width, height, e->pos, e->width, e->height)) e->Affect(this, p->make_bullet, p->make_ice);
		}
	}

	bool Hurt(int damage) {
		if (attacked_counter == 0) {
			if (damage!=0) mciSendString(_T("play hit from 0"), NULL, 0, NULL);
			hp -= damage;
			hurt = damage;
			hurt_counter = 20;
			attacked_counter = attacked_time;
			pas_vec->x = act_vec->x;
			pas_vec->y = act_vec->y;
			if (hp <= 0) alive = false;
		}
		return !alive;
	}

	bool CheckAlive() {
		return alive;
	}

public:
	int hp_max = 100;
	int hp = hp_max;
	int atk = 100;
	int speed_max = 4, speed = speed_max;
	bool alive = true;
	IMAGE hp_bk, hp_bar;
	TCHAR text[64];
private:
	const int FRAME_WIDTH = 135, FRAME_HEIGHT = 90;
	const int SHADOW_WIDTH = 40;
	Animation* anim_left;
	Animation* curr_anim;

	POINT pos = { 0,0 };
	POINT center = { pos.x + FRAME_WIDTH / 2,pos.y + FRAME_HEIGHT / 2 };
	bool facing_left = false;
	Vector* act_vec;
	Vector* pas_vec;
	int pos_shadow_x, pos_shadow_y;
	POINT check_pos;
	int hurt_counter = 0;
};
int generate_interval = 100;
void TryGenerateEnemy(vector<Enemy*>& enemy_list) {
	static int counter = 0;
	if ((++counter) % generate_interval == 0) enemy_list.push_back(new Enemy());
}

class Button {
public:
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed, int w = 0, int h = 0) {
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
		region = rect;
		if (w && h) {
			region.right = region.left + w;
			region.bottom = region.top + h;
			width = w;
			height = h;
		}
		else {
			width = img_idle.getwidth();
			height = img_idle.getheight();
		}
	}

	~Button() = default;

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
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
			putimage_alpha(region.left, region.top, &img_idle, width, height);
			break;
		case Status::Hovered:
			putimage_alpha(region.left, region.top, &img_hovered, width, height);
			break;
		case Status::Pushed:
			putimage_alpha(region.left, region.top, &img_pushed, width, height);
			break;
		}
		//rectangle(region.left, region.top, region.right, region.bottom);
	}

private:
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}

protected:
	virtual void OnClick() = 0;

protected:
	RECT region;
	IMAGE img_idle, img_hovered, img_pushed;
public:
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed
	};
	Status status = Status::Idle;
	int width, height;
};

class StartGameButton :public Button {
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed, int w = 0, int h = 0) :Button(rect, path_img_idle, path_img_hovered, path_img_pushed, w, h) {}
	~StartGameButton() = default;
protected:
	void OnClick() {
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

class QuitGameButton :public Button {
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed, int w = 0, int h = 0) :Button(rect, path_img_idle, path_img_hovered, path_img_pushed, w, h) {}
	~QuitGameButton() = default;
protected:
	void OnClick() {
		running = true;
	}
};

vector<wstring> skills = {
	L"攻击力+75",//0
	L"速度*150%\n生命上限+50",//1
	L"攻击*200%\n生命上限*50%", //2
	L"回复至生命上限",//3
	L"每次击杀，回复30生命",//4
	L"攻击范围*150%",//5
	L"攻击冷却*70%",//6
	L"击退*150%",//7
	L"每次击杀，召唤一个双倍伤害的魂火\n（可叠加个数）",//8
	L"攻击时，造成冰冻\n（可叠加时长）"//9
};
class OptionButton :public Button {
public:
	OptionButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed, int w = 0, int h = 0) :Button(rect, path_img_idle, path_img_hovered, path_img_pushed, w, h) {}
	~OptionButton() = default;
	void GetType(int t,Player* pl) { 
		type = t;
		p = pl;
		text_rect.top = region.top + 100;
		text_rect.left = region.left + 20;
		text_rect.bottom = region.bottom - 100;
		text_rect.right = region.right - 20;
		text_rect.top += (text_rect.bottom - text_rect.top) / 2 - 70;
	}
	void DrawTexts() {
		text[0] = '\0';
		//_stprintf_s(text, skills[type].c_str());
		_stprintf_s(text, _countof(text), _T("%s"), skills[type].c_str());

		settextstyle(50, 0, _T("IPix"));
		settextcolor(RGB(0, 0, 0));
		//outtextxy(region.left + 30, region.top + 100, text);
		drawtext(text, &text_rect, DT_WORDBREAK | DT_CENTER | DT_EDITCONTROL);
	}
	void KeyEvents(const ExMessage& msg) {
		
	}
	void OnClick() {
		p->status.to((int)Status::Idle);
		switch (type) {
		case 0:
			p->atk += 75;
			break;
		case 1:
			p->speed *= 1.5;
			p->hp_max += 50;
			p->hp += 50;
			break;
		case 2:
			p->atk *= 2;
			p->hp_max *= 0.5;
			break;
		case 3:
			p->hp = p->hp_max;
			break;
		case 4:
			p->hp_steal += 30;
			break;
		case 5:
			p->atk_width *= 1.5;
			p->atk_height *= 1.5;
			break;
		case 6:
			p->attack_cool *= 0.7;
			break;
		case 7:
			p->force *= 1.5;
			break;
		case 8:
			p->make_bullet++;
			break;
		case 9:
			p->make_ice++;
			break;
		}

		option = false;
		delete this;
	}
public:
	int type = 0;
	TCHAR text[64];
	RECT text_rect;
	Player* p;
};
vector<OptionButton*> options;


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

	RECT region_btn_start_game, region_btn_quit_game;
	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2 - BUTTON_WIDTH + 20;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 530;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;
	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2 + BUTTON_WIDTH - 20;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 530;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;
	StartGameButton btn_start_game(region_btn_start_game, _T("imgs/UI/ui_start_idle.png"), _T("imgs/UI/ui_start_hovered.png"), _T("imgs/UI/ui_start_pushed.png"));
	QuitGameButton btn_quit_game(region_btn_quit_game, _T("imgs/UI/ui_quit_idle.png"), _T("imgs/UI/ui_quit_hovered.png"), _T("imgs/UI/ui_quit_pushed.png"));

	IMAGE enemy_shadow;
	loadimage(&enemy_shadow, _T("imgs/Enemy/shadow_enemy.png"));
	loadimage(&img_background, _T("imgs/UI/background.png"));
	loadimage(&img_menu, _T("imgs/UI/menu.png"));
	loadimage(&img_bkg2, _T("imgs/UI/bkg2.png"));

	mciSendString(_T("open mus/the_new_puppet_full.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.mp3 type mpegvideo alias hit"), NULL, 0, NULL);
	mciSendString(_T("open mus/wave.mp3 alias wave"), NULL, 0, NULL);
	mciSendString(_T("open mus/hurt.mp3 alias hurt"), NULL, 0, NULL);
	
	//开启双缓冲
	BeginBatchDraw();
	//主循环
	while (running) {
		//开始计时
		start_time = GetTickCount();

		//选择强化
		if (option && options.empty()) {
			RECT option_rect;
			option_rect.top = 50;
			option_rect.left = 50;
			vector<int> pool;
			for (int i = 0; i < skills.size(); i++) pool.push_back(i);
			for (int i = 0; i < 3; i++) {
				options.push_back(new OptionButton(option_rect, _T("imgs/UI/option_idle.png"), _T("imgs/UI/option_hovered.png"), _T("imgs/UI/option_pushed.png"), 380, 620));
				int index = engine() % pool.size();
				options.back()->GetType(pool[index], &player);
				pool.erase(pool.begin() + index);
				option_rect.left += 20 + 380;
			}
		}
		else if (!option && !options.empty()) options.clear();

		//捕获事件
		while (peekmessage(&msg)) {
			if (is_game_started) {
				if (!option) player.ProcessEvent(msg);
				else {
					for (auto o : options) o->ProcessEvent(msg);
					if (msg.message == WM_KEYDOWN) {
						//123快速选择强化，空格跳过此次强化
						switch (msg.vkcode) {
						case 0x31:
							options[0]->status = Button::Status::Pushed;
							break;
						case 0x32:
							options[1]->status = Button::Status::Pushed;
							break;
						case 0x33:
							options[2]->status = Button::Status::Pushed;
							break;
						}
					}
					else if (msg.message == WM_KEYUP) {
						switch (msg.vkcode) {
						case 0x31:
							if (options[0]->status == Button::Status::Pushed) options[0]->OnClick();
							break;
						case 0x32:
							if (options[1]->status == Button::Status::Pushed) options[1]->OnClick();
							break;
						case 0x33:
							if (options[2]->status == Button::Status::Pushed) options[2]->OnClick();
							break;
						case VK_SPACE:
							option = false;
							break;
						}
					}
				}
			}
			else {
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}

		if (is_game_started && !option) {
			//处理事件
			player.Move();
			UpdateBullets(bullet_list, player);
			TryGenerateEnemy(enemy_list);

			for (Enemy* enemy : enemy_list) {
				enemy->Move(player);
				//检测碰撞
				if (enemy->CheckPlayerCollision(player) && player.Attackable()) {
					player.Hurt(enemy->atk);
					if (player.hp <= 0) {
						GameOver();
						break;
					}
					enemy->Hurt(0);
				}
				for (auto bullet = bullet_list.begin(); bullet != bullet_list.end();) {
					if (enemy->CheckBulletCollision(*bullet)) {
						enemy->Hurt(player.atk * 2);
						delete* bullet;
						/*bullet = */bullet_list.erase(bullet);
						break;
					}
					else bullet++;
				}
			}
			//移除死亡敌人
			for (int i = 0; i < enemy_list.size(); i++) {
				checked_enemy = enemy_list[i];
				if (!checked_enemy->CheckAlive()) {
					score++;
					experience++;
					player.hp += player.hp_steal;
					swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete checked_enemy;
					if (i > 0) i--;
					//强化角色
					if ((experience % (span + level) == 0 && level != 0) || (score == span && level == 0)) {
						option = true;
						level++;
						experience = 0;
					}
				}
			}
			//强化敌人
			enemy_hp = 100 * max((double)(difficulty / 7.0), 1);
			//enemy_atk = 100 * max((double)(difficulty / 20.0) - 1, 1);
			enemy_speed = 4 * min(max((double)(difficulty / 25.0), 1), 15);
			generate_interval = max(25, 100 - difficulty);
		}

		//清空窗口
		cleardevice();

		//开始绘图
		if (is_game_started) {
			if (!option) {
				putimage(0, 0, &img_background);
				player.Events();
				for (Bullet* bullet : bullet_list) bullet->Draw(&player);
				player.Draw(15);
				for (Enemy* enemy : enemy_list) {
					enemy->Events(&player);
					enemy->Draw(10, enemy_shadow);
				}
				DrawPlayerUI(score, &player);
			}
			else {
				putimage_alpha(0, 0, &img_bkg2, WINDOW_WIDTH, WINDOW_HEIGHT);
				for (auto o : options) {
					o->Draw();
					o->DrawTexts();
				}
			}
		}
		else {
			putimage_alpha(0, 0, &img_menu, WINDOW_WIDTH, WINDOW_HEIGHT);
			btn_start_game.Draw();
			btn_quit_game.Draw();
			player.GetAnim()->Play(WINDOW_WIDTH - 210 - 30, WINDOW_HEIGHT - 348, 10, 210, 348);
		}

		//while (option) {
		//	while (peekmessage(&msg)) {
		//		for (auto o : options) o->ProcessEvent(msg);
		//	}
		//	cleardevice();
		//	putimage_alpha(0, 0, &img_bkg2, WINDOW_WIDTH, WINDOW_HEIGHT);
		//	for (auto o : options) o->Draw(380, 620);
		//	FlushBatchDraw();
		//}

		//刷新双缓冲
		FlushBatchDraw();

		//结束计时
		end_time = GetTickCount();
		//计算帧耗时并睡眠降帧，优化性能
		Sleep(fr_intv > (end_time - start_time) ? fr_intv - (end_time - start_time) : fr_intv);
	}
	//回收资源
	for (auto it : Atlas::all_atlases) delete it;
	for (auto it : Animation::all_animations) delete it;
	//结束双缓冲
	EndBatchDraw();
	closegraph();

	return 0;
}