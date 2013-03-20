typedef struct {
	u8 unk0[24];
	s32 unk24;
	s32 unk28;
	s32 unk32;
	s32 unk36;
	s32 unk40;
} Unk1;

// size 1024 * 100
typedef struct {
	u8 unk0[24];
	s32 unk24;
	u8 unk28[100];
	Unk1 unk128;
} Unk0;

static Unk0 *g_1750; // 1750
static s32 g_max; // 1754

void sub_00000000(s32 idx)
{
	Unk0 *p;

	if (g_1750 == NULL) {
		return;
	}

	if (g_max == 0) {
		return;
	}

	// ((idx < g_max) ^ 1) | (idx >> 31)
	if (idx < 0 || idx >= g_max) {
		return;
	}

	p = g_1750 + idx * 1024 * 100;

	if (p == NULL) {
		return;
	}

	if ((p->unk128.unk24 + 2 * p->unk128.unk28 - p->unk24 + 1600) < 1536) {
		sub_000000F8(idx, p->unk128.unk32);
	}

	if (p->unk128.unk28 < p->unk128.unk40) {
		return;
	}

	if (p->unk128.unk36 != (p->unk128.unk24 + p->unk128.unk28 * (p->unk128.unk32 + 1) + 1600) {
		return;
	}

	p->unk128.unk32 ^= 1;
	p->unk128.unk36 = p->unk128.unk24 + p->unk128.unk28 * p->unk128.unk32 + 1600;
}