#include "lab.h"

/* #region Constants */
/* #endregion Constants */
/* #region Types */
/* #endregion Types */
/* #region Helpers */
/* #endregion Helpers */
/* #region Functions */
#ifdef TEST
#define main main_exclude
#endif

int main(int argc, char *argv[])
{
    transfer_configuration configuration;
    transfer_parse_result result = parse_transfer_configuration(argc, argv, &configuration);
    if (result != TRANSFER_PARSE_OK)
    {
        print_transfer_usage();
        return result == TRANSFER_PARSE_USAGE ? 0 : 1;
    }
    if (configuration.mode == TRANSFER_SEND)
    {
        return run_sender_transfer(&configuration);
    }
    return run_receiver_transfer(&configuration);
}
/* #endregion Functions */
