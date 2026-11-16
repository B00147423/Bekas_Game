#pragma once
#include <raylib.h>
#include "ScreenManager.h"
#include "Lighting/GraphicsSettings.h"
namespace UI {
class SettingsScreen : public Screen {
public:
    SettingsScreen();
    void Update() override;
    void Draw() override;
    const Lighting::GraphicsSettings& GetGraphics() const { return m_graphics; }
    unsigned int Revision() const { return m_revision; }
    bool TakeBackRequest() { bool value=m_back; m_back=false; return value; }
private:
    Lighting::GraphicsSettings m_graphics;
    unsigned int m_revision=1;
    bool m_back=false;
    Rectangle Row(int index) const;
    void Save() const;
};
}
