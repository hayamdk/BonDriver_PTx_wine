#define PTXCTRL_FUNC __attribute__((__cdecl__))
//#define PTXCTRL_FUNC __declspec(dllimport)
//#define PTXCTRL_FUNC extern "C" __declspec(dllimport)
//#define PTXCTRL_FUNC __declspec(dllexport)
//#define PTXCTRL_FUNC extern "C" __declspec(dllexport)

typedef enum {
	ISDB_T = 0,
	ISDB_S = 1
} tuner_type_t;

typedef void* ptx_handle_t;

PTXCTRL_FUNC ptx_handle_t pt3_open(tuner_type_t tuner_type);
PTXCTRL_FUNC ptx_handle_t pt1_open(tuner_type_t tuner_type);
//PTXCTRL_FUNC int ptx_open_device(const char *dev);
//PTXCTRL_FUNC int ptx_close_device(int fd);
PTXCTRL_FUNC int ptx_close(ptx_handle_t handle);
PTXCTRL_FUNC int ptx_select(ptx_handle_t handle, int timeout_ms);
PTXCTRL_FUNC int ptx_read(ptx_handle_t handle, uint8_t *buf, int maxsize);
PTXCTRL_FUNC void ptx_purge(ptx_handle_t handle);
PTXCTRL_FUNC int ptx_tune(ptx_handle_t handle, FREQUENCY *freq);
PTXCTRL_FUNC double ptx_getlevel_t(ptx_handle_t handle);
PTXCTRL_FUNC double ptx_getlevel_s(ptx_handle_t handle);
PTXCTRL_FUNC int ptx_start(ptx_handle_t handle);
PTXCTRL_FUNC int ptx_stop(ptx_handle_t handle);
