#include "suite/ParamSpec.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace suite {

namespace {

std::string fmt (const char* fmt, double v)
{
	char buf[64];
	std::snprintf (buf, sizeof (buf), fmt, v);
	return buf;
}

std::string lower (std::string s)
{
	std::transform (s.begin (), s.end (), s.begin (),
	                [] (unsigned char c) { return static_cast<char> (std::tolower (c)); });
	return s;
}

std::string trim (const std::string& s)
{
	auto b = s.find_first_not_of (" \t\r\n");
	if (b == std::string::npos)
		return {};
	auto e = s.find_last_not_of (" \t\r\n");
	return s.substr (b, e - b + 1);
}

} // namespace

int32_t ParamSpec::stepCount () const
{
	switch (taper)
	{
		case Taper::Choice: return static_cast<int32_t> (choices.size ()) - 1;
		case Taper::Toggle: return 1;
		default: return 0;
	}
}

double ParamSpec::toPlain (double n) const
{
	n = std::clamp (n, 0.0, 1.0);
	switch (taper)
	{
		case Taper::Linear: return min + n * (max - min);
		case Taper::Log: return min * std::pow (max / min, n);
		case Taper::Power: return min + std::pow (n, curve) * (max - min);
		case Taper::Choice: return std::round (n * stepCount ());
		case Taper::Toggle: return n > 0.5 ? 1.0 : 0.0;
	}
	return n;
}

double ParamSpec::toNormalized (double p) const
{
	switch (taper)
	{
		case Taper::Linear:
			return std::clamp ((p - min) / (max - min), 0.0, 1.0);
		case Taper::Log:
			p = std::clamp (p, min, max);
			return std::log (p / min) / std::log (max / min);
		case Taper::Power:
			p = std::clamp (p, min, max);
			return std::pow ((p - min) / (max - min), 1.0 / curve);
		case Taper::Choice:
		{
			auto steps = stepCount ();
			return steps > 0 ? std::clamp (std::round (p), 0.0, double (steps)) / steps : 0.0;
		}
		case Taper::Toggle: return p > 0.5 ? 1.0 : 0.0;
	}
	return p;
}

std::string ParamSpec::format (double v) const
{
	switch (unit)
	{
		case Unit::Hz:
			if (v >= 10000.0)
				return fmt ("%.1f kHz", v / 1000.0);
			if (v >= 1000.0)
				return fmt ("%.2f kHz", v / 1000.0);
			if (v >= 100.0)
				return fmt ("%.0f Hz", v);
			return fmt ("%.1f Hz", v);
		case Unit::Decibel:
			if (v <= -120.0)
				return "-inf dB";
			if (std::abs (v) < 0.05)
				return "0.0 dB";
			return fmt ("%+.1f dB", v);
		case Unit::Percent: return fmt ("%.0f %%", v);
		case Unit::Milliseconds:
			if (v >= 1000.0)
				return fmt ("%.2f s", v / 1000.0);
			if (v >= 100.0)
				return fmt ("%.0f ms", v);
			if (v >= 10.0)
				return fmt ("%.1f ms", v);
			return fmt ("%.2f ms", v);
		case Unit::Ratio: return v >= 99.5 ? "inf : 1" : fmt ("%.1f : 1", v);
		case Unit::Semitones: return fmt ("%+.1f st", v);
		case Unit::Degrees: return fmt ("%.0f deg", v);
		case Unit::Choice:
		{
			auto i = static_cast<size_t> (std::clamp (std::round (v), 0.0, double (choices.size () - 1)));
			return choices.empty () ? std::string {} : choices[i];
		}
		case Unit::OnOff: return v > 0.5 ? "On" : "Off";
		case Unit::Pan:
			if (std::abs (v) < 0.5)
				return "C";
			return fmt (v < 0.0 ? "L %.0f" : "R %.0f", std::abs (v));
		case Unit::Lufs: return v <= min + 0.05 ? "-inf LUFS" : fmt ("%.1f LUFS", v);
		case Unit::LoudnessUnits: return fmt ("%.1f LU", v);
		case Unit::DbTruePeak: return v <= min + 0.05 ? "-inf dBTP" : fmt ("%.1f dBTP", v);
		case Unit::None: break;
	}
	return std::abs (v) >= 100.0 ? fmt ("%.1f", v) : fmt ("%.2f", v);
}

bool ParamSpec::parse (const std::string& raw, double& out) const
{
	auto text = lower (trim (raw));
	if (text.empty ())
		return false;

	if (unit == Unit::Choice)
	{
		for (size_t i = 0; i < choices.size (); ++i)
		{
			if (lower (choices[i]) == text)
			{
				out = double (i);
				return true;
			}
		}
		char* end = nullptr;
		auto idx = std::strtod (text.c_str (), &end);
		if (end == text.c_str ())
			return false;
		out = std::clamp (std::round (idx) - 1.0, 0.0, double (choices.size () - 1)); // 1-based for users
		return true;
	}
	if (unit == Unit::OnOff || taper == Taper::Toggle)
	{
		if (text == "on" || text == "1" || text == "true" || text == "yes")
			out = 1.0;
		else if (text == "off" || text == "0" || text == "false" || text == "no")
			out = 0.0;
		else
			return false;
		return true;
	}
	if (unit == Unit::Pan)
	{
		// "C", "L 30", "R30", or a signed number (-100 = hard left)
		if (text == "c" || text == "center" || text == "centre")
		{
			out = 0.0;
			return true;
		}
		double sign = 1.0;
		std::string rest = text;
		if (text[0] == 'l' || text[0] == 'r')
		{
			sign = text[0] == 'l' ? -1.0 : 1.0;
			rest = trim (text.substr (1));
		}
		char* panEnd = nullptr;
		const double v = std::strtod (rest.c_str (), &panEnd);
		if (panEnd == rest.c_str ())
			return false;
		out = std::clamp (sign * v, min, max);
		return true;
	}
	if (unit == Unit::Decibel && (text == "-inf" || text == "-inf db"))
	{
		out = min;
		return true;
	}

	char* end = nullptr;
	double v = std::strtod (text.c_str (), &end);
	if (end == text.c_str ())
		return false;
	std::string suffix = trim (end);

	// Unit suffixes the user may type: "2.5k", "2.5 khz", "1.2s"
	if (unit == Unit::Hz && !suffix.empty () && suffix[0] == 'k')
		v *= 1000.0;
	else if (unit == Unit::Milliseconds && (suffix == "s" || suffix == "sec"))
		v *= 1000.0;

	out = std::clamp (v, std::min (min, max), std::max (min, max));
	return true;
}

const ParamSpec* findSpec (const ParamTable& table, uint32_t id)
{
	for (auto& s : table)
		if (s.id == id)
			return &s;
	return nullptr;
}

ParamSpec makeLinear (uint32_t id, std::string name, Unit unit, double min, double max, double def)
{
	return {id, name, name, Taper::Linear, unit, min, max, def};
}

ParamSpec makeLog (uint32_t id, std::string name, Unit unit, double min, double max, double def)
{
	return {id, name, name, Taper::Log, unit, min, max, def};
}

ParamSpec makePower (uint32_t id, std::string name, Unit unit, double min, double max, double def,
                     double curve)
{
	ParamSpec s {id, name, name, Taper::Power, unit, min, max, def};
	s.curve = curve;
	return s;
}

ParamSpec makeChoice (uint32_t id, std::string name, std::vector<std::string> choices, int def)
{
	ParamSpec s {id, name, name, Taper::Choice, Unit::Choice, 0.0, double (choices.size () - 1),
	             double (def)};
	s.choices = std::move (choices);
	return s;
}

ParamSpec makeToggle (uint32_t id, std::string name, bool def)
{
	return {id, name, name, Taper::Toggle, Unit::OnOff, 0.0, 1.0, def ? 1.0 : 0.0};
}

ParamSpec makeBypass (uint32_t id)
{
	auto s = makeToggle (id, "Bypass", false);
	s.flags = kAutomatable | kBypass;
	return s;
}

} // namespace suite
