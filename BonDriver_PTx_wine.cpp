#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "ptx_ctrl.lib")
#pragma comment(lib, "shlwapi.lib")

//#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <shlwapi.h>

typedef	struct	_frequency{
	int frequencyno;
	int slot;
} FREQUENCY;

#include "ptx_ctrl.h"
#include "channel_list.h"

#include "IBonDriver.h"
#include "IBonDriver2.h"

#define BUFSIZE				(4*1024*1024)
#define MSG_SIZE			(188*256)

static int pt3_mode = 1;
static tuner_type_t tuner_type = ISDB_T;

/*static inline __int64 gettime()
{
	__int64 result;
	_timeb tv;
	_ftime(&tv);
	result = (__int64)tv.time * 1000;
	result += tv.millitm;
	return result;
}*/

typedef enum{
	NOT_TUNED,
	TUNE_SUCCESS,
	TUNE_FAILED
} tune_stat_t;

class CTCPcTuner : public IBonDriver2
{
public:
	CTCPcTuner();
	virtual ~CTCPcTuner();

	// IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);

	const BOOL SetChannel(const BYTE bCh);
	const float GetSignalLevel(void);

	const DWORD WaitTsStream(const DWORD dwTimeOut = 0);
	const DWORD GetReadyCount(void);

	const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain);
	const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain);

	void PurgeTsStream(void);

	// IBonDriver2(暫定)
	LPCTSTR GetTunerName(void);

	const BOOL IsTunerOpening(void);

	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);

	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

	const DWORD GetCurSpace(void);
	const DWORD GetCurChannel(void);

	void Release(void);

	/////

private:
	void read_all();
	void minimize_buf();

	uint8_t buf[BUFSIZE];
	int buf_pos;
	int buf_filled;

	int opened;
	tune_stat_t tune_stat;
	int started;
	DWORD curr_sp;
	DWORD curr_ch;
	ptx_handle_t handle;
};

extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{
	return (IBonDriver *)(new CTCPcTuner);
}

CTCPcTuner::CTCPcTuner()
{
	opened = 0;
	tune_stat = NOT_TUNED;
	started = 0;
	curr_sp = 0;
	curr_ch = 0;
	buf_filled = 0;
	buf_pos = 0;
}

CTCPcTuner::~CTCPcTuner()
{
}

const BOOL CTCPcTuner::OpenTuner()
{
	//printf("open tuner\n");
	ptx_handle_t ret;
	if (opened) {
		CloseTuner();
	}

	if (pt3_mode) {
		ret = pt3_open(tuner_type);
	} else {
		ret = pt1_open(tuner_type);
	}
	if (ret == NULL) {
		return FALSE;
	}

	handle = ret;
	opened = 1;
	return TRUE;
}

void CTCPcTuner::CloseTuner()
{
	//printf("close tuner\n");
	if (started) {
		ptx_stop(handle);
	}
	if (opened) {
		ptx_purge(handle);
		ptx_close(handle);
	}
	started = 0;
	tune_stat = NOT_TUNED;
	opened = 0;
	curr_sp = 0;
	curr_ch = 0;
	buf_filled = 0;
	buf_pos = 0;
}

const BOOL CTCPcTuner::SetChannel(const BYTE bCh)
{
	//printf("setchannel(1args)\n");
	return FALSE;
}

const float CTCPcTuner::GetSignalLevel()
{
	//printf("get siglevel\n");
	if ( !opened || tune_stat != TUNE_SUCCESS ) {
		return 0.0;
	}
	if (tuner_type == ISDB_T) {
		return (float)ptx_getlevel_t(handle);
	} else {
		return (float)ptx_getlevel_s(handle);
	}
}

const DWORD CTCPcTuner::WaitTsStream(const DWORD dwTimeOut)
{
	int ret;

	//printf("wait\n");

	if ( !opened || tune_stat != TUNE_SUCCESS ) {
		return WAIT_ABANDONED;
	}

	ret = ptx_select(handle, (int)dwTimeOut);
	if(ret > 0) {
		/* 読み出すデータがある */
		return WAIT_OBJECT_0;
	} else if(ret == 0) {
		/* タイムアウト */
		return WAIT_TIMEOUT;
	}
	/* エラー */
	return WAIT_FAILED;
}

void CTCPcTuner::read_all()
{
	int ret, buf_remain;

	if ( !opened || tune_stat != TUNE_SUCCESS ) {
		return;
	}

	while (buf_filled < BUFSIZE - MSG_SIZE) {
		if (WaitTsStream(0) != WAIT_OBJECT_0) {
			return;
		}
		buf_remain = (BUFSIZE - buf_filled) / 188 * 188;
		ret = ptx_read(handle, &buf[buf_filled], buf_remain);
		if (ret > 0) {
			buf_filled += ret;
		} else {
			return;
		}
	}
}

void CTCPcTuner::minimize_buf()
{
	if (buf_filled == buf_pos) {
		/* 残っているバッファが何もなければただちにポジションを0に戻す */
		buf_filled = 0;
		buf_pos = 0;
	}
	if (buf_filled >= BUFSIZE - MSG_SIZE * 8) {
		/* そうではなくバッファがほぼいっぱいに埋まっていたら最小化処理を行う */
		memmove(buf, &buf[buf_pos], buf_filled-buf_pos);
		buf_filled -= buf_pos;
		buf_pos = 0;
	}
}

const DWORD CTCPcTuner::GetReadyCount()
{
	int remain;

	//printf("ready count\n");

	if ( !opened || tune_stat != TUNE_SUCCESS ) {
		return 0;
	}

	remain = buf_filled - buf_pos;
	return remain / MSG_SIZE;
}

const BOOL CTCPcTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	BYTE *pSrc;
	BOOL ret;
	ret = GetTsStream(&pSrc, pdwSize, pdwRemain);
	//if (*pdwSize > MSG_SIZE) {
		memcpy(pDst, pSrc, *pdwSize);
	//}
	return ret;
}

const BOOL CTCPcTuner::GetTsStream(BYTE **pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	int remain;

	//printf("get ts\n");

	if ( !opened || tune_stat == TUNE_FAILED ) {
		*pdwSize = *pdwRemain = 0;
		return FALSE;
	}

	if (tune_stat == NOT_TUNED) {
		SetChannel(curr_ch, curr_sp);
		if (tune_stat == TUNE_FAILED) {
			*pdwSize = *pdwRemain = 0;
			return FALSE;
		}
	}

	if (!started) {
		if (ptx_start(handle) < 0) {
			*pdwSize = *pdwRemain = 0;
			return FALSE;
		}
		started = 1;
	}

	read_all();

	remain = buf_filled - buf_pos;
	*pDst = &buf[buf_pos];

	if (remain < MSG_SIZE) {
		*pdwSize = 0;
		*pdwRemain = 0;
		return FALSE;
	}

	buf_pos += MSG_SIZE;
	*pdwSize = MSG_SIZE;
	*pdwRemain = (remain / MSG_SIZE) - 1;

	minimize_buf();

	return TRUE;
}

void CTCPcTuner::PurgeTsStream()
{
	//printf("purge\n");
	if (opened && started) {
		/* この関数はstartedじゃないときに呼ぶと挙動が変 */
		ptx_purge(handle);
	}
	buf_pos = 0;
	buf_filled = 0;
}

void CTCPcTuner::Release()
{
}

LPCTSTR CTCPcTuner::GetTunerName()
{
	//printf("get tuner name\n");
	if (pt3_mode) {
		if (tuner_type == ISDB_T) { return L"PT3@wine:ISDB-T"; }
		else { return L"PT3@wine:ISDB-S"; }
	} else {
		if (tuner_type == ISDB_T) { return L"PT@wine:ISDB-T"; }
		else { return L"PT@wine:ISDB-S"; }
	}
}

const BOOL CTCPcTuner::IsTunerOpening()
{
	//printf("is tuner opening\n");
	return opened;
}

static tuner_space_t get_space(int n_sp)
{
	int i, t=0;
	for (i = 0; i < n_spaces; i++) {
		if (space_list[i].tuner_type == tuner_type) {
			if (t == n_sp) {
				return (tuner_space_t)i;
			}
			t++;
		}
	}
	return SPACE_INVALID;
}

static const channel_def_t* get_channel(int n_sp, int n_ch)
{
	int i, t=0;
	tuner_space_t sp = get_space(n_sp);
	if (sp == SPACE_INVALID) {
		return NULL;
	}
	for (i = 0; i < n_channels; i++) {
		if (channel_list[i].space == sp) {
			if (t == n_ch) {
				return &channel_list[i];
			}
			t++;
		}
	}
	return NULL;
}

LPCTSTR CTCPcTuner::EnumTuningSpace(const DWORD dwSpace)
{
	//printf("enum space\n");
	int sp = get_space((int)dwSpace);
	if (sp == SPACE_INVALID) {
		return NULL;
	}
	return space_list[sp].space_name;
}

LPCTSTR CTCPcTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	//printf("enum channel\n");
	const channel_def_t *ch = get_channel((int)dwSpace, (int)dwChannel);
	if (!ch) {
		return NULL;
	}
	return ch->channel_name;
}

const BOOL CTCPcTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	FREQUENCY freq;
	const channel_def_t *ch = get_channel((int)dwSpace, (int)dwChannel);
	if (!ch) {
		//printf("invalid channel\n");
		return FALSE;
	}

	if (!opened) {
		//printf("not opened\n");
		return FALSE;
	}

	if ( dwSpace == curr_sp && dwChannel == curr_ch && tune_stat == TUNE_SUCCESS ) {
		//printf("pass!(set channel)\n");
		return TRUE;
	}

	curr_sp = dwSpace;
	curr_ch = dwChannel;

	if (started) {

		if (ptx_stop(handle) < 0) {
			//printf("ptx_stop() failed! @ SetChannel\n");
			return FALSE;
		}
		started = 0;
	}

	freq.frequencyno = ch->freq;
	freq.slot = ch->slot;
	if (ptx_tune(handle, &freq) < 0) {
		tune_stat = TUNE_FAILED;
		//printf("tune failed!\n");
	} else {
		tune_stat = TUNE_SUCCESS;
		//printf("tune success!\n");
	}
	return TRUE;
}

const DWORD CTCPcTuner::GetCurSpace(void)
{
	if (opened) {
		return curr_sp;
	}
	return 0;
}

const DWORD CTCPcTuner::GetCurChannel(void)
{
	if (opened) {
		return curr_ch;
	}
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	WCHAR dllname[_MAX_PATH];

	if (fdwReason == DLL_PROCESS_ATTACH) {
		GetModuleFileName(hinstDLL, dllname, _MAX_PATH - 1);
		PathStripPath(dllname);
		//printf("%S\n", dllname);
		if (wcsncmp(dllname, L"BonDriver_PT3wine-T", wcslen(L"BonDriver_PT3wine-T")) == 0) {
			pt3_mode = 1;
			tuner_type = ISDB_T;
		} else if (wcsncmp(dllname, L"BonDriver_PT3wine-S", wcslen(L"BonDriver_PT3wine-S")) == 0) {
			pt3_mode = 1;
			tuner_type = ISDB_S;
		} else if (wcsncmp(dllname, L"BonDriver_PTwine-T", wcslen(L"BonDriver_PTwine-T")) == 0) {
			pt3_mode = 0;
			tuner_type = ISDB_T;
		} else if (wcsncmp(dllname, L"BonDriver_PTwine-S", wcslen(L"BonDriver_PTwine-S")) == 0) {
			pt3_mode = 0;
			tuner_type = ISDB_S;
		}
	}
	return TRUE;
}
