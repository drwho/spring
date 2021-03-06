#ifndef HUD_DRAWER_HDR
#define HUD_DRAWER_HDR

class CUnit;
struct HUDDrawer {
public:
	HUDDrawer(): draw(true) {}
	void Draw(const CUnit*);

	void SetDraw(bool b) { draw = b; }
	bool GetDraw() const { return draw; }

	static HUDDrawer* Get();

private:
	void PushState();
	void PopState();

	void DrawModel(const CUnit*);
	void DrawUnitDirectionArrow(const CUnit*);
	void DrawCameraDirectionArrow(const CUnit*);
	void DrawWeaponStates(const CUnit*);
	void DrawTargetReticle(const CUnit*);

	bool draw;
};

#define hudDrawer (HUDDrawer::Get())

#endif
