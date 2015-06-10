//#define PTXCTRL_FUNC __attribute__((__cdecl__))
//#define PTXCTRL_FUNC __declspec(dllimport)
#define PTXCTRL_FUNC extern "C" __declspec(dllimport)
//#define PTXCTRL_FUNC __declspec(dllexport)
//#define PTXCTRL_FUNC extern "C" __declspec(dllexport)

typedef enum {
	ISDB_T = 0,
	ISDB_S = 1
} tuner_type_t;

PTXCTRL_FUNC int pt3_open(tuner_type_t tuner_type);
PTXCTRL_FUNC int pt1_open(tuner_type_t tuner_type);
PTXCTRL_FUNC int ptx_open_device(const char *dev);
PTXCTRL_FUNC int ptx_close_device(int fd);
PTXCTRL_FUNC int ptx_select(int fd, int timeout_ms);
PTXCTRL_FUNC int ptx_read(int fd, uint8_t *buf, int maxsize);
PTXCTRL_FUNC void ptx_purge(int fd);
PTXCTRL_FUNC int ptx_tune(int fd, FREQUENCY *freq);
PTXCTRL_FUNC double ptx_getlevel_t(int fd);
PTXCTRL_FUNC double ptx_getlevel_s(int fd);
PTXCTRL_FUNC int ptx_start(int fd);
PTXCTRL_FUNC int ptx_stop(int fd);