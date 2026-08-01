// SPDX-License-Identifier: MIT
// Generated from villith/relink-logs English game-data catalogs at c1b5a32d6671ff4d710bfe2a228a70c20351311f.
// Legacy-only hashes are retained for compatibility.

#include "gbfr/skill_types.hpp"

#include <cctype>

namespace gbfr {

namespace {

constexpr SkillEntry kSkills[] = {
    {0x50079A1C, "SKILL_000_00", "ATK"},
    {0xF372F096, "SKILL_001_00", "HP"},
    {0x7279E478, "SKILL_002_00", "(Unused) DEF"},
    {0x8D78A19B, "SKILL_003_00", "Critical Hit Rate"},
    {0xCEB700EE, "SKILL_004_00", "Stun Power"},
    {0x3F488339, "SKILL_005_00", "Enmity"},
    {0x2FC8FBFF, "SKILL_006_00", "Stamina"},
    {0x9B90820C, "SKILL_007_00", "(Unused)"},
    {0x1C360C63, "SKILL_008_00", "Charged Attack DMG"},
    {0x3FEC5F80, "SKILL_009_00", "Linked Together"},
    {0x6D5ACD5C, "SKILL_010_00", "(Unused)"},
    {0x98ACE800, "SKILL_011_00", "(Unused)"},
    {0x8D078597, "SKILL_012_00", "Throw DMG"},
    {0xC0979A17, "SKILL_013_00", "Critical Hit DMG"},
    {0x6B694D6D, "SKILL_014_00", "Weak Point DMG"},
    {0x54401E12, "SKILL_015_00", "(Unused DMG counter)"},
    {0x3D718078, "SKILL_016_00", "(Unused DMG on Link)"},
    {0xA7A45F28, "SKILL_017_00", "Combo Finisher DMG"},
    {0xB360801D, "SKILL_018_00", "Concentrated Fire"},
    {0x0E592AC4, "SKILL_019_00", "(Unused DMG)"},
    {0xDC584F60, "SKILL_020_00", "DMG Cap"},
    {0x6CD9DBB8, "SKILL_021_00", "(Unused)"},
    {0x48069644, "SKILL_022_00", "(Unused)"},
    {0xCAC6AFF2, "SKILL_023_00", "(Unused)"},
    {0xF17850B9, "SKILL_024_00", "Combo Booster"},
    {0x2982C763, "SKILL_025_00", "(Unused)"},
    {0xFE02D02F, "SKILL_026_00", "(Unused DEF)"},
    {0x71F11A9B, "SKILL_027_00", "Tyranny"},
    {0xC35B111B, "SKILL_028_00", "Lucky Charge"},
    {0x4F1A3683, "SKILL_029_00", "Injury to Insult"},
    {0xA9D17F55, "SKILL_030_00", "Overdrive Assassin"},
    {0xAC9674C1, "SKILL_031_00", "Break Assassin"},
    {0xA254AB8C, "SKILL_032_00", "(Unused)"},
    {0xBC90C6A4, "SKILL_033_00", "(Unused)"},
    {0x998A78CD, "SKILL_034_00", "(Unused)"},
    {0xBEB4D1E9, "SKILL_035_00", "(Unused DEF)"},
    {0xE6CDBA9C, "SKILL_036_00", "Garrison"},
    {0x06F7CEDE, "SKILL_037_00", "(Unused DEF E0)"},
    {0x709B29B6, "SKILL_038_00", "(Unused DEF E1)"},
    {0xF8F6FD8E, "SKILL_039_00", "(Unused DEF E2)"},
    {0xE338C0E0, "SKILL_040_00", "(Unused DEF E3)"},
    {0x687202B5, "SKILL_041_00", "(Unused DEF E4)"},
    {0xD5A33083, "SKILL_042_00", "(Unused DEF E5)"},
    {0x130111F4, "SKILL_043_00", "(Unused)"},
    {0xA1A8E39D, "SKILL_044_00", "Stout Heart"},
    {0xE69A4694, "SKILL_045_00", "Guts"},
    {0x973B49AF, "SKILL_046_00", "Poison Resistance"},
    {0x7C84A6B3, "SKILL_047_00", "Burn Resistance"},
    {0xE22E33A8, "SKILL_048_00", "(Unused)"},
    {0x81A3BBB4, "SKILL_049_00", "(Unused)"},
    {0xC7848E9E, "SKILL_050_00", "(Unused)"},
    {0xD54F8CA7, "SKILL_051_00", "Sandtomb Resistance"},
    {0xFB572681, "SKILL_052_00", "Glaciate Resistance"},
    {0xF2FDA35D, "SKILL_053_00", "(Unused)"},
    {0x3759A5B9, "SKILL_054_00", "Dizzy Resistance"},
    {0x2242921F, "SKILL_055_00", "Paralysis Resistance"},
    {0xD62AC108, "SKILL_056_00", "(Unused)"},
    {0x50B453DD, "SKILL_057_00", "Skill Sealed Resistance"},
    {0xCFB48782, "SKILL_058_00", "SBA Sealed Resistance"},
    {0xBAA020D8, "SKILL_059_00", "(Unused)"},
    {0x0AA20846, "SKILL_060_00", "Improved Guard"},
    {0x3C2B57B0, "SKILL_061_00", "Guard Payback"},
    {0xE0A7A887, "SKILL_062_00", "(Unused)"},
    {0x8B3BF60C, "SKILL_063_00", "Improved Dodge"},
    {0x7C2E4D64, "SKILL_064_00", "Dodge Payback"},
    {0x9389CC06, "SKILL_065_00", "Improved Healing"},
    {0x6085DA25, "SKILL_066_00", "Regen"},
    {0x7CCFF74F, "SKILL_067_00", "Drain"},
    {0x95F3FA86, "SKILL_068_00", "Autorevive"},
    {0x318D12E9, "SKILL_069_00", "Quick Cooldown"},
    {0x05F2ECDC, "SKILL_070_00", "Cascade"},
    {0xC1F72F43, "SKILL_071_00", "(Unused)"},
    {0xB5FF9FD3, "SKILL_072_00", "Uplift"},
    {0x24883AF3, "SKILL_073_00", "Potion Hoarder"},
    {0xE833D96A, "SKILL_074_00", "(Unused)"},
    {0x3DDA90FA, "SKILL_075_00", "(Unused)"},
    {0x7EDE8000, "SKILL_076_00", "(Unused)"},
    {0xDC607D75, "SKILL_077_00", "Low Profile"},
    {0x6018372B, "SKILL_078_00", "Provoke"},
    {0xF687C5EF, "SKILL_079_00", "Fast Learner"},
    {0xC86F3082, "SKILL_080_00", "Rupie Tycoon"},
    {0xB9C865C1, "SKILL_081_00", "(Unused)"},
    {0x2816B618, "SKILL_082_00", "(Unused Normal Attack DMG)"},
    {0xEAE321EB, "SKILL_083_00", "Skilled Assault"},
    {0xF69D1076, "SKILL_084_00", "(Unused SBA DMG)"},
    {0xE0ABFDFE, "SKILL_085_00", "Aegis"},
    {0xA2FA9685, "SKILL_086_00", "Slow Resistance"},
    {0xB6E31F76, "SKILL_087_00", "Firm Stance"},
    {0x9702860F, "SKILL_088_00", "Blight Resistance"},
    {0x935D84E7, "SKILL_089_00", "(Unused DMG)"},
    {0xC1DC2F4A, "SKILL_090_00", "(Unused Perfect DMG)"},
    {0xA1E395FA, "SKILL_091_00", "(Unused ATK on hit)"},
    {0x10810867, "SKILL_092_00", "(Unused)"},
    {0xC7B4AAB4, "SKILL_093_00", "(Unused)"},
    {0x0053599E, "SKILL_094_00", "Steady Focus"},
    {0x1D02DF7D, "SKILL_095_00", "(Unused)"},
    {0x1470F860, "SKILL_096_00", "Steel Nerves"},
    {0x9189DE53, "SKILL_097_00", "(Unused)"},
    {0x323F6C09, "SKILL_098_00", "(Unused)"},
    {0x4B400B01, "SKILL_099_00", "(Unused Weak Crit)"},
    {0x3F2C482E, "SKILL_100_00", "Supplements"},
    {0x734C58D1, "SKILL_101_00", "(Unused Cut Area Dur)"},
    {0x5209463B, "SKILL_102_00", "(Unused Rampart Dur)"},
    {0x0EAD65E0, "SKILL_103_00", "Natural Defenses"},
    {0x09AA7DB5, "SKILL_104_00", "Nimble Defense"},
    {0x7D99DBD6, "SKILL_105_00", "(Unused)"},
    {0xD2C8E10A, "SKILL_106_00", "Nimble Onslaught"},
    {0x29B292A8, "SKILL_107_00", "Precise Resilience"},
    {0x52692E71, "SKILL_108_00", "(Unused)"},
    {0x7EDD69D0, "SKILL_109_00", "Precise Wrath"},
    {0x8F502F0D, "SKILL_110_00", "Life on the Line"},
    {0x84078CB0, "SKILL_111_00", "Quick Charge"},
    {0xD0A1C6E5, "SKILL_112_00", "(Unused)"},
    {0x57E8A93F, "SKILL_113_00", "Sigil Booster"},
    {0xCD030268, "SKILL_114_00", "Fearless Drive"},
    {0xA38510E2, "SKILL_114_01", "Fearless Spirit"},
    {0xDADE14DC, "SKILL_114_02", "Fearless Heart"},
    {0x3BFED918, "SKILL_115_00", "Guardian's Conviction"},
    {0xF8496336, "SKILL_115_01", "Guardian's Honor"},
    {0x9AFDFA9E, "SKILL_115_02", "Guardian's Warpath"},
    {0x151E4674, "SKILL_116_00", "Helmsman's Navigation"},
    {0xA374FDF0, "SKILL_116_01", "Helmsman's Tenacity"},
    {0xD76F4D24, "SKILL_116_02", "Helmsman's Warpath"},
    {0xB48EEF48, "SKILL_117_00", "Mage's Aspiration"},
    {0x11AAE5F5, "SKILL_117_01", "Mage's Savvy"},
    {0xC00163B3, "SKILL_117_02", "Mage's Warpath"},
    {0xAA83F548, "SKILL_118_00", "Veteran's Insight"},
    {0x921B6B0C, "SKILL_118_01", "Veteran's Vision"},
    {0x0E42BE1B, "SKILL_118_02", "Veteran's Warpath"},
    {0x23D0F67F, "SKILL_119_00", "Rose's Blooming"},
    {0xC2A4C7A9, "SKILL_119_01", "Rose's Profusion"},
    {0x8519AD4A, "SKILL_119_02", "Rose's Warpath"},
    {0xD908223D, "SKILL_120_00", "Phantasm's Concord"},
    {0x7351D602, "SKILL_120_01", "Phantasm's Harmony"},
    {0xA339D642, "SKILL_120_02", "Phantasm's Warpath"},
    {0x8CDF9382, "SKILL_121_00", "White Dragon's Oath"},
    {0xD1012D8C, "SKILL_121_01", "White Dragon's Glory"},
    {0x6316CBEB, "SKILL_121_02", "White Dragon's Warpath"},
    {0x2E65A774, "SKILL_122_00", "Hero's Creed"},
    {0x16EFF868, "SKILL_122_01", "Hero's Will"},
    {0xD8F66C1C, "SKILL_122_02", "Hero's Warpath"},
    {0xE60A735C, "SKILL_123_00", "Lord's Procession"},
    {0x6FF05223, "SKILL_123_01", "Lord's Ambition"},
    {0xBA504607, "SKILL_123_02", "Lord's Warpath"},
    {0x86CBCDC4, "SKILL_124_00", "Dragonslayer's Dominance"},
    {0x05FA4599, "SKILL_124_01", "Dragonslayer's Ingenuity"},
    {0xC7D379F1, "SKILL_124_02", "Dragonslayer's Warpath"},
    {0x9A9DC170, "SKILL_125_00", "Holy Knight's Luster"},
    {0x522E2388, "SKILL_125_01", "Holy Knight's Grandeur"},
    {0xB85202BC, "SKILL_125_02", "Holy Knight's Warpath"},
    {0x0CD6C625, "SKILL_126_00", "Swordmaster's Prowess"},
    {0xA3B49220, "SKILL_126_01", "Swordmaster's Art"},
    {0xDAEFBB27, "SKILL_126_02", "Swordmaster's Warpath"},
    {0x29B07BEB, "SKILL_127_00", "Butterfly's Grace"},
    {0xA63B89CD, "SKILL_127_01", "Butterfly's Valor"},
    {0xFDD1AD24, "SKILL_127_02", "Butterfly's Warpath"},
    {0x5463232F, "SKILL_128_00", "Eternal Rage's Mettle"},
    {0x451D814C, "SKILL_128_01", "Eternal Rage's Ethos"},
    {0x0F026CF0, "SKILL_128_02", "Eternal Rage's Warpath"},
    {0xEC3CF174, "SKILL_129_00", "Founder's Strategy"},
    {0xAF513A9D, "SKILL_129_01", "Founder's Truth"},
    {0xE6B92E34, "SKILL_129_02", "Founder's Warpath"},
    {0x93A2093C, "SKILL_130_00", "Versalis Foundation"},
    {0x7AD0C010, "SKILL_130_01", "Versalis Ignition"},
    {0xB064A634, "SKILL_130_02", "Versalis Heart"},
    {0x6EBFA176, "SKILL_131_00", "Crimson's Clout"},
    {0xF1D5DBD0, "SKILL_131_01", "Crimson's Flight"},
    {0x4F135217, "SKILL_131_02", "Crimson's Warpath"},
    {0x7440E869, "SKILL_132_00", "Ebony's Presence"},
    {0xCD124165, "SKILL_132_01", "Ebony's Poise"},
    {0xD7F9BB88, "SKILL_132_02", "Ebony's Warpath"},
    {0x0FBA47E8, "SKILL_133_00", "Fortifying Vigor (JP HP+/Rec)"},
    {0xA4D6B880, "SKILL_134_00", "Instilling Vigor (JP EXP)"},
    {0xCDEB73F6, "SKILL_135_00", "Gilding Vigor (JP Rupies+)"},
    {0x1DC9D7E7, "SKILL_136_00", "Held Under Resistance"},
    {0xDD4A701E, "SKILL_137_00", "Darkflame Resistance"},
    {0x4BF2E191, "SKILL_138_00", "ATK↓ Resistance"},
    {0x66DE60B1, "SKILL_139_00", "DEF↓ Resistance"},
    {0x082033CB, "SKILL_140_00", "Crabby Resonance"},
    {0x1B0D9897, "SKILL_141_00", "Crabvestment Returns"},
    {0xD461ECFB, "SKILL_141_04", "Crabvestment Returns"},
    {0x9AD8B5E6, "SKILL_142_00", "Seven Net"},
    {0x40223C28, "SKILL_143_00", "Catastrophe"},
    {0x1E1CECCE, "SKILL_143_10", "Catastrophe Nova"},
    {0x74AA75D6, "SKILL_144_00", "Stronghold"},
    {0xDC225C96, "SKILL_145_00", "Power Hungry"},
    {0x4C588C27, "SKILL_146_00", "War Elemental"},
    {0x5E422AE5, "SKILL_147_00", "Path to Mastery"},
    {0xAF794A87, "SKILL_150_00", "Untouchable"},
    {0x57AB5B10, "SKILL_151_00", "Supplementary DMG"},
    {0x82CE278D, "SKILL_152_00", "Less Is More"},
    {0x1568E0E4, "SKILL_153_00", "Head Start"},
    {0x70395731, "SKILL_154_00", "Berserker"},
    {0xCD18A77D, "SKILL_156_00", "Potent Greens"},
    {0x333E5862, "SKILL_157_00", "Roll of the Die"},
    {0xA8A3163B, "SKILL_158_00", "Glass Cannon"},
    {0xEC1C6779, "SKILL_159_00", "Flight over Fight"},
    {0xDBE1D775, "SKILL_160_00", "Alpha"},
    {0x8D2ADB6E, "SKILL_161_00", "Beta"},
    {0x5C862E13, "SKILL_162_00", "Gamma"},
    {0xD3B8C21F, "SKILL_164_00", "Crabmiration"},
    {0x48A95B8D, "SKILL_166_00", "Greater Aegis"},
    {0xF71F8997, "SKILL_167_00", "Auto Potion"},
    {0x77C809F5, "SKILL_170_00", "Spirit Edge's Rally"},
    {0x9230E3F5, "SKILL_170_01", "Spirit Edge's Fury"},
    {0x7B4FC47A, "SKILL_170_02", "Spirit Edge's Warpath"},
    {0xEF05EC4D, "SKILL_170_03", "Seven-Star Boundary"},
    {0xE85FF8E0, "SKILL_171_00", "Dark Huntress's Volley"},
    {0x8572B8AF, "SKILL_171_01", "Dark Huntress's Surge"},
    {0x81B293D9, "SKILL_171_02", "Dark Huntress's Warpath"},
    {0x281214AB, "SKILL_171_03", "Two-Crown Boundary"},
    {0xD40D1E9B, "SKILL_172_00", "Supreme Primarch's Awe"},
    {0x15806DFC, "SKILL_172_01", "Supreme Primarch's Nimbus"},
    {0x4E5F6706, "SKILL_172_02", "Supreme Primarch's Warpath"},
    {0x1A2EF59E, "SKILL_172_03", "Ain"},
    {0x1DE14C65, "SKILL_173_00", "Gladiator's Frenzy"},
    {0x26956F25, "SKILL_173_01", "Gladiator's Top"},
    {0xDBA19768, "SKILL_173_02", "Gladiator's Warpath"},
    {0x7B5B081D, "SKILL_174_00", "Bladequeen's Serenade"},
    {0x9ACE140B, "SKILL_174_01", "Bladequeen's Circuit"},
    {0x79266456, "SKILL_174_02", "Bladequeen's Warpath"},
    {0xD176D262, "SKILL_175_00", "Ultramarine's Flash"},
    {0x461A8E07, "SKILL_175_01", "Ultramarine's Adversity"},
    {0xB953CC1E, "SKILL_175_02", "Ultramarine's Warpath"},
    {0x7D75D904, "SKILL_176_00", "Thunderwolf's Recharge"},
    {0xBE3404B9, "SKILL_176_01", "Thunderwolf's Acuity"},
    {0x3EB345D7, "SKILL_176_02", "Thunderwolf's Warpath"},
    {0x47384248, "SKILL_177_00", "Enchantress's Blessing"},
    {0x30773197, "SKILL_177_01", "Enchantress's Rhythm"},
    {0x807B6684, "SKILL_177_02", "Enchantress's Warpath"},
    {0x06719232, "SKILL_178_00", "The Black's Mark"},
    {0xED8D8AD8, "SKILL_178_01", "The Black's Impulse"},
    {0x5559232F, "SKILL_178_02", "The Black's Warpath"},
    {0xD3047326, "SKILL_230_00", "(Unused MaxATK Cap)"},
    {0xED3A49A1, "SKILL_231_00", "(Unused MaxHP Cap)"},
    {0x259F189B, "SKILL_232_00", "(Unused Link Time)"},
    {0xEE85CD1F, "SKILL_233_00", "Berserker Echo"},
    {0x3D8153A1, "SKILL_234_00", "Spartan Echo"},
    {0x51C115D2, "SKILL_235_00", "Super Ultimate Perfect Dodge"},
    {0xBF78FBFC, "SKILL_301_00", "Immortal Shell"},
    {0x46EE3116, "SKILL_302_00", "In a Pinch"},
    {0x89C66ACB, "SKILL_303_00", "Sumo Force"},
    {0x3B71AF12, "SKILL_311_00", "DMG Cap Ecru"},
    {0xFFF8CF64, "SKILL_312_00", "DMG Cap Sage"},
    {0x235D86EF, "SKILL_313_00", "Supernova"},
    {0xAEFEB1BC, "SKILL_314_00", "DMG Cap Cobalt"},
    {0x0151CF9E, "SKILL_315_00", "DMG Cap Cardinal"},
    {0xBBD77C33, "SKILL_316_00", "Unbound Strike"},
    {0x020DB733, "SKILL_317_00", "Unbound Technique"},
    {0x3F682593, "SKILL_318_00", "Unbound Exertion"},
    {0x79027FC8, "SKILL_319_00", "Unbound Master"},
    {0x0DE887A0, "SKILL_320_00", "Celestial Nyx"},
    {0xA7726190, "SKILL_321_00", "Celestial Lumen"},
    {0x9232DC17, "SKILL_322_00", "Celestial Terra"},
    {0x36E3848D, "SKILL_323_00", "Celestial Incendo"},
    {0xA898E283, "SKILL_324_00", "Celestial Aqua"},
    {0xD029FE08, "SKILL_325_00", "Fatebreaker"},
    {0x73220725, "SKILL_326_00", "Celestial Ventus"},
    {0xF26BAEA5, "SKILL_327_00", "Divergence"},
};

constexpr bool skill_hashes_are_valid() {
    for (const auto& entry : kSkills) {
        if (StringHash32(entry.asset_id).value() != entry.hash) return false;
    }
    return true;
}

static_assert(skill_hashes_are_valid(), "skill catalog hash mismatch");

} // namespace

const SkillEntry* find_skill(std::uint32_t hash) noexcept {
    for (const auto& entry : kSkills) {
        if (entry.hash == hash) return &entry;
    }
    return nullptr;
}

bool ascii_iequals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto lower_a = static_cast<char>(
            std::tolower(static_cast<unsigned char>(left[i])));
        const auto lower_b = static_cast<char>(
            std::tolower(static_cast<unsigned char>(right[i])));
        if (lower_a != lower_b) return false;
    }
    return true;
}

const SkillEntry* find_skill_by_name(
    std::string_view value, std::uint32_t preferred_hash) noexcept {
    if (value.empty()) return nullptr;

    for (const auto& entry : kSkills) {
        if (ascii_iequals(entry.asset_id, value)) return &entry;
    }

    const SkillEntry* match = nullptr;
    bool ambiguous = false;
    for (const auto& entry : kSkills) {
        if (!ascii_iequals(entry.name, value)) continue;
        if (entry.hash == preferred_hash) return &entry;
        if (match == nullptr) match = &entry;
        else ambiguous = true;
    }
    return ambiguous ? nullptr : match;
}

const SkillEntry* find_skill_by_name(std::string_view value) noexcept {
    return find_skill_by_name(value, 0);
}

} // namespace gbfr
