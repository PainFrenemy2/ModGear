#include "InfoServer.h"


std::vector<AudioInfo> aTemp;
std::string currentVersion = MODGEAR_VERSION_STRING;
std::set<int> connectedPlayers;

const char* tagsname[] =
{
    "icon_admin", "icon_stunned", "icon_stunned_1", "icon_stunned_2", "icon_stunned_3", "icon_pensioner",
    "flag_am", "flag_am", "flag_by", "flag_ca", "flag_de", "flag_kz", "flag_md", "flag_ro", "flag_ru", "flag_tj", "flag_ua", "flag_unk", "flag_usa",
    "icon_lvl_vip_1", "icon_lvl_vip_2", "icon_lvl_vip_3", "icon_lvl_vip_4", "icon_lvl_vip_5", "icon_lvl_vip_6", "icon_lvl_vip_7", "icon_lvl_vip_8", "icon_lvl_vip_9", "icon_lvl_vip_10",
    "flag_am_vip", "flag_by_vip", "flag_ca_vip", "flag_de_vip", "flag_kz_vip", "flag_md_vip", "flag_ro_vip", "flag_ru_vip", "flag_tj_vip", "flag_ua_vip", "flag_unk_vip", "flag_usa_vip",
    "icon_vip_new"
};
const char* regionNames[] = {
	"california", "alabama", "alaska", "arizona", "arkansas", "colorado",
	"connecticut", "delaware", "florida", "georgia", "hawaii", "idaho", "illinois",
	"indiana", "iowa", "kansas", "maine", "maryland", "massachusetts", "michigan",
	"minnesota", "mississippi", "missouri", "montana", "nebraska", "nevada",
	"new hampshire", "new jersey", "new mexico", "new york", "north carolina",
	"north dakota", "ohio", "Oklahoma", "oregon", "Pennsylvania", "rhode island",
	"south carolina", "south dakota", "tennessee", "texas", "utah", "vermont",
	"washington", "wisconsin", "wyoming"
};
// все диски клиента (data/maps/veh_mods/veh_mods.ide) и текстура обода, которую красит SetVehicleWheelColor
// Текстуры взяты из моделей в models/tuning.img. "" - у диска нет своей текстуры, цвет не ставится
const WheelInfo wheelTable[] = {
	{ 1025, "" }, // wheel_or1 (обычный диск GTA, своей текстуры нет)
	{ 1073, "" }, // wheel_sr6 (обычный диск GTA, своей текстуры нет)
	{ 1074, "" }, // wheel_sr3 (обычный диск GTA, своей текстуры нет)
	{ 1075, "" }, // wheel_sr2 (обычный диск GTA, своей текстуры нет)
	{ 1076, "" }, // wheel_lr4 (обычный диск GTA, своей текстуры нет)
	{ 1077, "" }, // wheel_lr1 (обычный диск GTA, своей текстуры нет)
	{ 1078, "" }, // wheel_lr3 (обычный диск GTA, своей текстуры нет)
	{ 1079, "" }, // wheel_sr1 (обычный диск GTA, своей текстуры нет)
	{ 1080, "" }, // wheel_sr5 (обычный диск GTA, своей текстуры нет)
	{ 1081, "" }, // wheel_sr4 (обычный диск GTA, своей текстуры нет)
	{ 1082, "" }, // wheel_gn1 (обычный диск GTA, своей текстуры нет)
	{ 1083, "" }, // wheel_lr2 (обычный диск GTA, своей текстуры нет)
	{ 1084, "" }, // wheel_lr5 (обычный диск GTA, своей текстуры нет)
	{ 1085, "" }, // wheel_gn2 (обычный диск GTA, своей текстуры нет)
	{ 1096, "" }, // wheel_gn3 (обычный диск GTA, своей текстуры нет)
	{ 1097, "" }, // wheel_gn4 (обычный диск GTA, своей текстуры нет)
	{ 1098, "" }, // wheel_gn5 (обычный диск GTA, своей текстуры нет)
	{ 6258, "cir_diff" }, // wheel_r1
	{ 6259, "ccr_diff" }, // wheel_s6
	{ 6260, "rsclassic_diff" }, // wheel_s3
	{ 6261, "xt006_diff" }, // wheel_s2
	{ 6262, "57c_diff" }, // wheel_l4
	{ 6264, "305m_diff" }, // wheel_l1
	{ 6266, "505_diff" }, // wheel_l3
	{ 6272, "lmgt4_diff" }, // wheel_s1
	{ 6276, "fhurt_diff" }, // wheel_s5
	{ 6278, "tech7_diff" }, // wheel_s4
	{ 6660, "techmesh_diff" }, // wheel_g1
	{ 6694, "vishunu_diff" }, // wheel_l2
	{ 6695, "vfs2_diff" }, // wheel_l5
	{ 6697, "cv5_diff" }, // wheel_g2
	{ 6747, "vsmpog_diff" }, // wheel_g3
	{ 6748, "wire_diff" }, // wheel_g4
	{ 6749, "d9r_diff" }, // wheel_g5
	{ 6750, "blq_diff" }, // wheel_vip1
	{ 6751, "bth_diff" }, // wheel_vip2
	{ 6752, "cvt_diff" }, // wheel_vip3
	{ 6753, "hre201_diff" }, // wheel_vip4
	{ 6754, "lexani_diff" }, // wheel_vip5
	{ 6755, "adv1_diff" }, // wheel_vip6
	{ 6756, "af134_diff" }, // wheel_vip7
	{ 6757, "dubp_diff" }, // wheel_vip8
	{ 6758, "dubbc_diff" }, // wheel_vip9
	{ 6759, "bcslayer_diff" }, // wheel_vip10
	{ 6760, "scooby_diff" }, // wheel_scooby
	{ 6761, "wheel_bergm" }, // wheel_bergm
	{ 6762, "wheel_darkblade" }, // wheel_darkblade
	{ 6763, "wheel_smsr" }, // wheel_smsr
	{ 6764, "wheel_splnces" }, // wheel_splnces
	{ 6765, "wheel_tenzor" }, // wheel_tenzor
	{ 6766, "wheel_vskf" }, // wheel_vskf
	{ 6769, "wheel_watanabe" }, // wheel_watanabe
	{ 6770, "wheel_wolfrace" }, // wheel_wolfrace
	{ 6771, "wheel1" }, // wheel_1
	{ 6772, "wheel2" }, // wheel_2
	{ 6773, "wheel3" }, // wheel_3
	{ 6774, "wheel4" }, // wheel_4
	{ 6775, "wheel5" }, // wheel_5
	{ 6776, "wheel6" }, // wheel_6
	{ 6777, "wheel7" }, // wheel_7
	{ 6778, "wheel8" }, // wheel_8
	{ 6779, "wheel9" }, // wheel_9
	{ 6780, "wheel10" }, // wheel_10
	{ 6781, "wheel11" }, // wheel_11
	{ 6782, "wheel12" }, // wheel_12
	{ 6783, "wheel13" }, // wheel_13
	{ 6784, "wheel14" }, // wheel_14
	{ 6785, "wheel15" }, // wheel_15
	{ 6786, "wheel16" }, // wheel_16
	{ 6787, "wheel17" }, // wheel_17
	{ 12377, "obj79_af140_tx" }, // wheel_af140
	{ 12378, "obj79_af141_tx" }, // wheel_af141
	{ 12379, "obj79_lex_fire_tx" }, // wheel_lex_fire
	{ 12380, "obj79_lex_myst_tx" }, // wheel_lex_myst
	{ 12381, "obj79_tf_brake_tx" }, // wheel_tf_brake
	{ 12382, "" }, // wheel_tf_donut (обычный диск GTA, своей текстуры нет)
	{ 12383, "obj79_tf_lollipop_tx" }, // wheel_tf_lollipop
	{ 12384, "obj79_tf_pizza_tx" }, // wheel_tf_pizza
	{ 12385, "obj79_tf_wave_tx" }, // wheel_tf_wave
	{ 12386, "obj79_tf_zombie_tx" }, // wheel_tf_zombie
	{ 12387, "obj79_tf_main_tx" }, // wheel_turbofan_def
	{ 12556, "wheel_donk" }, // wheel_donk
	{ 12557, "wheel_meshes" }, // wheel_meshes
	{ 12558, "wheel_uzi" }, // wheel_uzi
	{ 12559, "wheel_weed" }, // wheel_weed
	{ 15655, "wheel_sf1" }, // wheel_sf1
	{ 15656, "wheel_sf2" }, // wheel_sf2
	{ 15657, "wheel_sf3" }, // wheel_sf3
	{ 15658, "wheel_sf4" }, // wheel_sf4
	{ 15284, "wheel_f1" }, // wheel_f1
	{ 15285, "wheel_f2" }, // wheel_f2
	{ 15286, "wheel_f3" }, // wheel_f3
	{ 12637, "wheel_hlw1" }, // wheel_hlw1
	{ 12638, "wheel_hlw2" }, // wheel_hlw2
	{ 14115, "wheel_ny1" }, // wheel_ny1
	{ 14116, "wheel_ny2" }, // wheel_ny2
	{ 12578, "wheel_mansfv10" }, // wheel_mansfv10
	{ 12503, "" }, // wheel_tahoest (текстуры: un_tyreciv un_generic)
	{ 12467, "fporto_rimman" }, // wheel_fporto
	{ 12412, "wheel_mesa" }, // wheel_mesa
	{ 15602, "patriot_wheel2" }, // wheel_patriot
	{ 15527, "charsrt_copwheel" }, // wheel_charsrt
	{ 12401, "crvnvic_wheel" }, // wheel_crvnvic
	{ 14319, "wheel_x100mk2" }, // wheel_x100mk2
	{ 15671, "l35bs_badges" }, // wheel_l35bs
	{ 15672, "wheel_fc3sbs" }, // wheel_fc3sbs
	{ 15689, "wheel_tsoarerbs" }, // wheel_tsoarerbs
};
const int wheelTableSize = sizeof(wheelTable) / sizeof(wheelTable[0]);

const WheelInfo* FindWheel(int model)
{
	for (const WheelInfo& w : wheelTable) if (w.model == model) return &w;
	return nullptr;
}

const char* tonerNames[] = { "wnd", "wdor_f", "wdor_r", "wnd_rr", "wndfrl", "wndrrl", "wsup" };
const char* wheelSettings[] = { "wheel_lf_dummy", "wheel_rf_dummy", "wheel_lb_dummy", "wheel_rb_dummy" };

PlayerInfos PI[1000];

VehicleInfo vTemp[2000];

InfoServer iServer;

const ColorRGB ColorRGBArray[] =
{
	{0, 0, 0}, {245, 245, 245}, {161, 119, 42}, {16, 4, 132}, {57, 55, 38}, {110, 68, 134}, {16, 142, 215},
	{183, 117, 76}, {198, 190, 189}, {114, 112, 94}, {122, 89, 70}, {121, 106, 101}, {141, 126, 93},
	{90, 89, 88}, {214, 218, 214}, {163, 161, 156}, {63, 95, 51}, {26, 14, 115}, {42, 10, 123}, {148, 157, 159},
	{120, 78, 59}, {62, 46, 115}, {59, 30, 105}, {140, 145, 150}, {89, 84, 81}, {69, 62, 63}, {167, 169, 165},
	{90, 92, 99}, {104, 74, 61}, {146, 149, 151}, {33, 31, 66}, {43, 39, 95}, {171, 148, 132}, {124, 123, 118},
	{100, 100, 100}, {82, 87, 90}, {39, 37, 37}, {53, 58, 45}, {150, 163, 147}, {136, 122, 109}, {24, 25, 34},
	{95, 103, 111}, {42, 28, 124}, {21, 10, 95}, {38, 56, 25}, {32, 27, 93}, {114, 152, 157}, {96, 117, 122},
	{134, 149, 152}, {176, 176, 173}, {136, 137, 132}, {69, 79, 48}, {104, 98, 77}, {72, 34, 22}, {75, 47, 39},
	{86, 98, 125}, {171, 164, 158}, {113, 141, 156}, {34, 24, 109}, {129, 104, 78}, {152, 156, 156}, {71, 115, 145},
	{38, 28, 102}, {159, 157, 148}, {165, 167, 164}, {70, 140, 142}, {30, 26, 52}, {140, 122, 106}, {142, 173, 170},
	{143, 152, 171}, {46, 31, 133}, {151, 130, 111}, {83, 88, 88}, {144, 167, 154}, {35, 26, 96}, {44, 32, 32}, {150, 160, 164},
	{132, 157, 170}, {43, 34, 120}, {109, 49, 14}, {63, 42, 114}, {94, 113, 123}, {40, 29, 116}, {50, 46, 30}, {47, 50, 77},
	{68, 27, 124}, {32, 91, 46}, {131, 90, 57}, {55, 40, 109}, {143, 162, 167}, {177, 177, 175}, {85, 65, 54}, {110, 108, 109},
	{137, 106, 15}, {107, 75, 32}, {87, 62, 43}, {157, 159, 155}, {149, 132, 108}, {149, 132, 77}, {127, 155, 174}, {143, 108, 64},
	{59, 37, 31}, {118, 146, 171}, {115, 69, 19}, {108, 129, 150}, {106, 104, 100}, {130, 80, 16}, {131, 153, 161}, {148, 86, 56},
	{97, 86, 82}, {86, 105, 127}, {154, 146, 140}, {135, 110, 89}, {50, 53, 71}, {79, 98, 68}, {39, 10, 115}, {87, 52, 34}, {27, 13, 100},
	{198, 173, 163}, {83, 88, 105}, {128, 139, 155}, {28, 11, 98}, {94, 93, 91}, {40, 68, 98}, {39, 24, 115}, {109, 55, 27}, {174, 106, 236},
	{0, 0, 0}, {23, 117, 23}, {6, 6, 33}, {120, 84, 18}, {13, 42, 69}, {30, 30, 87}, {1, 7, 1}, {90, 34, 37},
	{170, 137, 44}, {189, 77, 138}, {58, 150, 53}, {183, 183, 183}, {141, 76, 70}, {140, 136, 132}, {103, 120, 129},
	{38, 122, 129}, {111, 80, 106}, {111, 62, 88}, {114, 185, 140}, {120, 79, 130}, {106, 39, 109}, {19, 29, 30}, {6, 19, 30}, {24, 37, 31},
	{49, 69, 44}, {153, 76, 30}, {67, 95, 46}, {72, 153, 30}, {153, 153, 30}, {118, 153, 153}, {153, 132, 124}, {30, 46, 153}, {8, 30, 44}, {7, 36, 20},
	{77, 62, 153}, {153, 76, 30}, {129, 129, 25}, {42, 41, 26}, {111, 97, 22}, {135, 102, 27}, {153, 63, 108}, {14, 26, 72}, {153, 115, 122}, {153, 109, 116}, {126, 56, 83},
	{7, 36, 34}, {12, 25, 62}, {14, 33, 70}, {30, 30, 153}, {141, 76, 141}, {128, 91, 128}, {126, 62, 123}, {55, 23, 60}, {23, 53, 115}, {24, 24, 120}, {26, 52, 131}, {28, 47, 142}, {83, 62, 126}, {124, 109, 124}, {2, 12, 2}, {7, 36, 7},
	{18, 48, 22}, {27, 48, 22}, {79, 43, 100}, {82, 132, 54}, {144, 149, 153}, {150, 141, 129}, {30, 153, 153}, {76, 153, 127}, {146, 146, 131}, {34, 130, 120}, {153, 60, 43}, {11, 58, 58}, {78, 121, 138}, {73, 31, 14}, {28, 55, 21}, {58, 39, 21}, {117, 87, 55}, {32, 8, 6}, {38, 19, 7}, {75, 57, 32}, {137, 80, 44}, {108, 66, 21}, {80, 50, 16}, {99, 22, 36}, {21, 32, 105}, {148, 141, 140}, {19, 96, 81}, {2, 15, 9}, {58, 87, 140}, {142, 136, 82}, {82, 92, 153}, {30, 88, 153}, {99, 58, 153}, {78, 143, 153}, {30, 49, 153}, {66, 24, 13}, {30, 30, 82}, {13, 66, 66}, {30, 153, 76}, {29, 42, 8}, {29, 130, 150}, {25, 127, 25}, {31, 20, 59}, {23, 82, 116}, {141, 63, 137}, {108, 26, 126}, {11, 55, 11}, {13, 69, 39}, {36, 31, 7}, {115, 69, 120}, {58, 101, 138}, {23, 38, 115}, {144, 148, 49}, {29, 148, 86}, {61, 22, 89}, {47, 138, 27}, {11, 22, 56}, {4, 24, 4}, {142, 93, 53}, {91, 63, 46}, {40, 26, 86}, {39, 14, 78}, {103, 108, 112}, {66, 62, 59}, {51, 45, 46}, {125, 126, 123}, {66, 68, 74}, {78, 52, 40}
};
const ChatMod chatMod[] =
{
	{0, {0, 223, 255}, "vr", "Ваше сообщение будет отправлено в вип-чат", "Вип-чат"},
	{1, {45, 176, 67}, "r", "Ваше сообщение будет отправлено в чат организации", "Орг. чат"},
	{2, {45, 176, 67}, "rb", "Ваше сообщение будет отправлено в нонРП-чат организации", "Орг. нрп"},
	{3, {101, 187, 133}, "fam", "Ваше сообщение будет отправлено в семейный чат", "Сем. чат"},
	{4, {175, 175, 183}, "b", "Ваше сообщение будет отправлено в нонРП чат", "нонРП чат"},
	{5, {140, 230, 240}, "s", "Вы прокричите Ваше сообщение", "Кричать"},
	{6, {193, 176, 148}, "с", "Вы прошепчете Ваше сообщение", "Шептать"},
	{7, {0, 215, 255}, "j", "Ваше сообщение будет отправлено в рабочую рацию", "Раб. чат"}
};