#include "live_view.h"
// TODO: Redo methods using default parameters as shown in get_live_view_data

// Error codes
#define LIBPTP2_CANNOT_CONNECT  1
#define LIBPTP2_NO_DEVICE       2
#define LIBPTP2_ALREADY_OPEN    3
#define LIBPTP2_NOT_OPEN        4

// PTP Stuff
enum PTP_CONTAINER_TYPE {
    PTP_CONTAINER_TYPE_COMMAND  = 1,
    PTP_CONTAINER_TYPE_DATA     = 2,
    PTP_CONTAINER_TYPE_RESPONSE = 3,
    PTP_CONTAINER_TYPE_EVENT    = 4
};

enum CHDK_OPERATIONS {
    CHDK_OP_VERSION     = 0,
    CHDK_OP_GET_MEMORY,
    CHDK_OP_SET_MEMORY,
    CHDK_OP_CALL_FUNCTION,
    CHDK_OP_TEMP_DATA,
    CHDK_OP_UPLOAD_FILE,
    CHDK_OP_DOWNLOAD_FILE,
    CHDK_OP_EXECUTE_SCRIPT,
    CHDK_OP_SCRIPT_STATUS,
    CHDK_OP_SCRIPT_SUPPORT,
    CHDK_OP_READ_SCRIPT_MSG,
    CHDK_OP_WRITE_SCRIPT_MSG,
    CHDK_OP_GET_DISPLAY_DATA
};

enum CHDK_TYPES {
    CHDK_TYPE_UNSUPPORTED = 0,
    CHDK_TYPE_NIL,
    CHDK_TYPE_BOOLEAN,
    CHDK_TYPE_INTEGER,
    CHDK_TYPE_STRING,
    CHDK_TYPE_TABLE
};

enum CHDK_TEMP_DATA {
    CHDK_TEMP_DOWNLOAD = 1, // Download data instead of upload
    CHDK_TEMP_CLEAR
};

enum CHDK_SCRIPT_LANGAUGE {
    CHDK_LANGUAGE_LUA   = 0,
    CHDK_LANGUAGE_UBASIC
};

// Placeholder structs
struct ptp_command {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transaction_id;
    char * payload;
};

struct ptp_response {
    int x;
};

struct script_return {
    int x;
};

struct lv_data {
    int x;
};

struct param_container {
    unsigned int length;
    unsigned short type;
    unsigned short code;
    unsigned int transaction_id;
};

// Have to define the helper class first, or I can't use it in CameraBase
class PTPContainer {
    private:
        static const uint32_t default_length = sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint16_t)+sizeof(uint16_t);
        uint32_t length;
        unsigned char * payload;    // We'll deal with this completely internally
        void init();
    public:
        uint16_t type;
        uint16_t code;
        uint32_t transaction_id;    // We'll end up setting this externally
        PTPContainer();
        PTPContainer(uint16_t type, uint16_t op_code);
        PTPContainer(unsigned char * data);
        ~PTPContainer();
        void add_param(uint32_t param);
        void set_payload(unsigned char * payload, int payload_length);
        unsigned char * pack();
        unsigned char * get_payload(int * size_out);  // This might end up being useful...
        uint32_t get_length();  // So we can get, but not set
        void unpack(unsigned char * data);
};

class LVData {
    lv_data_header * vp_head;
    lv_framebuffer_desc * fb_desc;
    uint8_t * payload;
    void init();
    static uint8_t clip(int v);
    static void yuv_to_rgb(uint8_t **dest, uint8_t y, int8_t u, int8_t v);
    public:
        LVData();
        LVData(uint8_t * payload, int payload_size);
        ~LVData();
        void read(uint8_t * payload, int payload_size);
        uint8_t * get_rgb(int * out_size, int * out_width, int * out_height, bool skip=false);    // Some cameras don't require skip
};

class CameraBase {
    private:
        libusb_device_handle *handle;
        int usb_error;
        struct libusb_interface_descriptor *intf;
        uint8_t ep_in;
        uint8_t ep_out;
        uint32_t _transaction_id;
        void init();
    protected:
        int _bulk_write(unsigned char * bytestr, int length, int timeout);
        int _bulk_write(unsigned char * bytestr, int length);
        int _bulk_read(unsigned char * data_out, int size, int * transferred, int timeout);
        int _bulk_read(unsigned char * data_out, int size, int * transferred);
    public:
        CameraBase();
        CameraBase(libusb_device *dev);
        ~CameraBase();
        bool open(libusb_device *dev);
        bool close();
        bool reopen();
        int send_ptp_message(unsigned char * bytestr, int size, int timeout);
        int send_ptp_message(unsigned char * bytestr, int size);
        int send_ptp_message(PTPContainer cmd, int timeout);
        int send_ptp_message(PTPContainer cmd);
        int send_ptp_message(PTPContainer * cmd, int timeout);
        int send_ptp_message(PTPContainer * cmd);
        void recv_ptp_message(PTPContainer *out, int timeout);
        void recv_ptp_message(PTPContainer *out);
        // TODO: Does C++ allow a different way of doing "default" parameter values?
        void ptp_transaction(PTPContainer *cmd, PTPContainer *data, bool receiving, PTPContainer *out_resp, PTPContainer *out_data, int timeout);
        void ptp_transaction(PTPContainer *cmd, PTPContainer *data, bool receiving, PTPContainer *out_resp, PTPContainer *out_data);
        static libusb_device * find_first_camera();
        int get_usb_error();
        unsigned char * pack_ptp_command(struct ptp_command * cmd);
        int get_and_increment_transaction_id(); // What a beautiful name for a function
};

class PTPCamera : public CameraBase {
    public:
        PTPCamera();
};

class CHDKCamera : public CameraBase {
    void yuv_live_to_cd_rgb(const char *p_yuv, unsigned buf_width, unsigned buf_height, unsigned x_offset, unsigned y_offset, unsigned width, unsigned height, int skip, uint8_t *r, uint8_t *g, uint8_t *b);
    static uint8_t clip_yuv(int v);
    static uint8_t yuv_to_r(uint8_t y, int8_t v);
    static uint8_t yuv_to_g(uint8_t y, int8_t u, int8_t v);
    static uint8_t yuv_to_b(uint8_t y, int8_t u);
    struct filebuf * _pack_file_for_upload(char * local_filename, char * remote_filename);
    struct filebuf * _pack_file_for_upload(char * local_filename);
    public:
        CHDKCamera();
        CHDKCamera(libusb_device *dev);
        float get_chdk_version(void);
        uint32_t check_script_status(void);
        uint32_t execute_lua(char * script, uint32_t * script_error, bool block);
        uint32_t execute_lua(char * script, uint32_t * script_error);
        void read_script_message(PTPContainer * out_data, PTPContainer * out_resp);
        uint32_t write_script_message(char * message, uint32_t script_id);
        uint32_t write_script_message(char * message);
        bool upload_file(char * local_filename, char * remote_filename, int timeout);
        char * download_file(char * filename, int timeout);
        void get_live_view_data(LVData * data_out, bool liveview=true, bool overlay=false, bool palette=false);
        char * _wait_for_script_return(int timeout);
};
