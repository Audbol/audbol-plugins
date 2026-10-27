#include "UtlEditor.h"

#include "UtlViews.h"

#include "suite/Brand.h"

#include "vstgui/lib/cframe.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace suite::ui;

namespace utl {

namespace {
constexpr int kEditorWidth = 1000;
constexpr int kEditorHeight = 540;
constexpr float kTick = 0.033f;
constexpr double kSmoothing = 0.9; // ~300 ms at 30 Hz

float toDb (float g) { return g > 1e-6f ? 20.0f * std::log10 (g) : -120.0f; }
} // namespace

UtlEditor::UtlEditor (UtlController* controller) : EditorBase (controller, kEditorWidth, kEditorHeight) {}

void UtlEditor::build (CFrame*)
{
	add (new HeaderBar (CRect (0, 0, kEditorWidth, 44), SUITE_NAME, "Utility"));
	add (new Label (CRect (520, 12, 590, 32), "Channels", 11, colors::textDim, 2));
	bind (add (new Segmented (CRect (596, 11, 896, 33), this, kChannelMode, this)), kChannelMode);
	bind (add (new Toggle (CRect (912, 11, 986, 33), this, kBypass, "BYPASS")), kBypass)->setAccent (colors::warning);

	scope = add (new Goniometer (CRect (12, 54, 342, 374)));
	correlation = add (new BipolarMeter (CRect (352, 54, 760, 210), "CORRELATION", "-1", "+1", true));
	balance = add (new BipolarMeter (CRect (352, 218, 760, 374), "BALANCE", "L", "R", false));

	add (new Panel (CRect (770, 54, 988, 374), colors::panel, 8));
	const char* names[] = {"L", "R", "M", "S"};
	for (int i = 0; i < 4; ++i)
	{
		const CCoord x = 788 + i * 50;
		meters[i] = add (new LevelMeter (CRect (x, 66, x + 28, 362), names[i], LevelMeter::Mode::Level, 60.0));
	}

	add (new Panel (CRect (12, 384, 988, 528), colors::panel, 8));
	auto gainKnob = bind (add (new Knob (CRect (28, 404, 118, 504), this, kGain, this)), kGain);
	gainKnob->setBipolar (true);
	bind (add (new Toggle (CRect (138, 420, 208, 444), this, kMute, "MUTE")), kMute)->setAccent (colors::danger);
	bind (add (new Toggle (CRect (138, 452, 208, 476), this, kDcFilter, "DC")), kDcFilter);

	auto widthKnob = bind (add (new Knob (CRect (238, 404, 328, 504), this, kWidth, this)), kWidth);
	widthKnob->setBipolar (true); // grows from 100 %
	auto panKnob = bind (add (new Knob (CRect (348, 404, 438, 504), this, kPan, this)), kPan);
	panKnob->setBipolar (true);
	bind (add (new Toggle (CRect (458, 420, 538, 444), this, kInvertL, "INV L")), kInvertL);
	bind (add (new Toggle (CRect (458, 452, 538, 476), this, kInvertR, "INV R")), kInvertR);

	bind (add (new Toggle (CRect (568, 420, 668, 444), this, kBassMono, "BASS MONO")), kBassMono);
	bassFreq = bind (add (new Knob (CRect (680, 404, 770, 504), this, kBassMonoFreq, this)), kBassMonoFreq);
	add (new Label (CRect (790, 410, 980, 426), "Bass Mono removes stereo", 10, colors::textFaint, 0));
	add (new Label (CRect (790, 428, 980, 444), "below the set frequency", 10, colors::textFaint, 0));
	add (new Label (CRect (790, 456, 980, 472), "Width 0% = mono, 200% = wide", 10, colors::textFaint, 0));
}

void UtlEditor::teardown ()
{
	scope = nullptr;
	correlation = balance = nullptr;
	for (auto& m : meters)
		m = nullptr;
	bassFreq = nullptr;
}

void UtlEditor::tick ()
{
	if (!scope)
		return;
	const auto s = utlCtrl ()->takeStats ();
	const float peaks[4] = {s.peakL, s.peakR, s.peakM, s.peakS};
	for (int i = 0; i < 4; ++i)
		meters[i]->update (s.fresh ? toDb (peaks[i]) : -120.0f, kTick);

	utlCtrl ()->copyScope (scopeL, scopeR);
	scope->update (scopeL, scopeR);

	char buf[48];
	const double energy = s.sumLL + s.sumRR;
	if (s.fresh && energy > 1e-9)
	{
		// Correlation: +1 mono, 0 unrelated, -1 out of phase.
		const double corr = s.sumLR / std::sqrt (std::max (s.sumLL * s.sumRR, 1e-18));
		corrSmoothed = kSmoothing * corrSmoothed + (1.0 - kSmoothing) * std::clamp (corr, -1.0, 1.0);
		// Balance: -1 all left .. +1 all right, by energy.
		const double bal = (s.sumRR - s.sumLL) / energy;
		balanceSmoothed = kSmoothing * balanceSmoothed + (1.0 - kSmoothing) * bal;
	}
	correlation->setValue (corrSmoothed);
	std::snprintf (buf, sizeof (buf), "%+.2f", corrSmoothed);
	const char* status = corrSmoothed < 0.0   ? "Out of phase: will cancel in mono"
	                     : corrSmoothed < 0.3 ? "Wide: check in mono"
	                                          : "Mono compatible";
	correlation->setReadout (buf, s.fresh || energy > 0 ? status : "");

	balance->setValue (balanceSmoothed);
	if (std::abs (balanceSmoothed) < 0.01)
		std::snprintf (buf, sizeof (buf), "C");
	else
	{
		// Express as the level difference between the sides
		const double ratio = (1.0 + std::abs (balanceSmoothed)) / std::max (1e-6, 1.0 - std::abs (balanceSmoothed));
		std::snprintf (buf, sizeof (buf), "%s %.1f dB", balanceSmoothed < 0 ? "L" : "R", 10.0 * std::log10 (ratio));
	}
	balance->setReadout (buf);

	bassFreq->setAlphaValue (ctrl ()->getPlain (kBassMono) > 0.5 ? 1.0f : 0.4f);
}

} // namespace utl
