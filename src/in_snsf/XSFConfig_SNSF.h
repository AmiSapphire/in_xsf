/*
 * xSF - SNSF configuration
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 *
 * Partially based on the vio*sf framework
 *
 * NOTE: 16-bit sound is always enabled, the player is currently limited to
 * creating a 16-bit PCM for Winamp and can not handle 8-bit sound from
 * snes9x.
 */

#pragma once

#include <bitset>
#include <string>
#include "windowsh_wrapper.h"
#include "XSFConfig.h"

class wxWindow;
class XSFConfigDialog;
class XSFPlayer;

class XSFConfig_SNSF : public XSFConfig
{
protected:
	static constexpr bool /*initSixteenBitSound = true, */initReverseStereo = false;
	static constexpr unsigned initResampler = 1;
	inline static const std::string initMutes = "00000000";

	friend class XSFConfig;
	bool /*sixteenBitSound, */reverseStereo;
	std::bitset<8> mutes;

	XSFConfig_SNSF();
	void LoadSpecificConfig() override;
	void SaveSpecificConfig() override;
	void InitializeSpecificConfigDialog(XSFConfigDialog *dialog) override;
	void ResetSpecificConfigDefaults(XSFConfigDialog *dialog) override;
	void SaveSpecificConfigDialog(XSFConfigDialog *dialog) override;
	void CopySpecificConfigToMemory(XSFPlayer *xSFPlayer, bool preLoad) override;
public:
	unsigned resampler;

	void About(HWND parent) override;
	XSFConfigDialog *CreateDialogBox(wxWindow *window, const std::string &title) override;
};
