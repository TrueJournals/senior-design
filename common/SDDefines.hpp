#ifndef SDDEFINES_HPP_
#define SDDEFINES_HPP_

#define SD_MAGIC 0xF061

enum SD_COMMANDS {
    SD_REQ_CONNECTED = 1,
    SD_IS_CONNECTED,
    SD_NOT_CONNECTED,
    SD_JOYDATA,
    SD_LVDATA,
    SD_QUIT,
    SD_UPDATE,
    SD_OK,
    SD_ERROR,
};

#endif /* SDDEFINES_HPP_ */
