
#if defined(HAVE_CONFIG_H)
#include "config_ac.h"
#endif

#include <stdlib.h>

#include "log.h"
#include "libipm.h"
#include "trans.h"
#include "os_calls.h"
#include "string_calls.h"

#include "test_libipm.h"

struct trans *g_t_out = NULL;
struct trans *g_t_in = NULL;
struct trans *g_t_vanilla = NULL;

const char *g_supported_types = "ybnqiuxtsdhogB";
const char *g_unimplemented_types = "dhog";

/******************************************************************************/
static const char *
msgno_to_str(unsigned short msgno)
{
    return (msgno == TEST_MESSAGE_NO) ? "TEST_MESSAGE_NO" : NULL;
}

/******************************************************************************/
static void
suite_test_libipm_calls_start(void)
{
    int sck[2];
    int istatus;
    struct trans *t1 = NULL;
    struct trans *t2 = NULL;
    struct trans *t3 = NULL;
    int success = 0;

    if ((t1 = trans_create(TRANS_MODE_UNIX, 128, 128)) == NULL)
    {
        const char *errstr = g_get_strerror();
        LOG(LOG_LEVEL_ERROR, "Can't create test transport 1 [%s]", errstr);
    }
    else if ((t2 = trans_create(TRANS_MODE_UNIX, 128, 128)) == NULL)
    {
        const char *errstr = g_get_strerror();
        LOG(LOG_LEVEL_ERROR, "Can't create test transport 2 [%s]", errstr);
    }
    else if ((t3 = trans_create(TRANS_MODE_UNIX, 128, 128)) == NULL)
    {
        const char *errstr = g_get_strerror();
        LOG(LOG_LEVEL_ERROR, "Can't create test transport 3 [%s]", errstr);
    }
    else if ((istatus = g_sck_local_socketpair(sck)) < 0)
    {
        const char *errstr = g_get_strerror();
        LOG(LOG_LEVEL_ERROR, "Can't create test sockets [%s]", errstr);
    }
    else
    {
        enum libipm_status init1;
        enum libipm_status init2;

        g_sck_set_non_blocking(sck[1]);

        t1->sck = sck[0];
        t1->type1 = TRANS_TYPE_CLIENT;
        t1->status = TRANS_STATUS_UP;

        t2->sck = sck[1];
        t2->type1 = TRANS_TYPE_SERVER;
        t2->status = TRANS_STATUS_UP;

        init1 = libipm_init_trans(t1, LIBIPM_FAC_TEST, msgno_to_str);
        init2 = libipm_init_trans(t2, LIBIPM_FAC_TEST, msgno_to_str);
        if (init1 != E_LI_SUCCESS || init2 != E_LI_SUCCESS)
        {
            LOG(LOG_LEVEL_ERROR, "libipm_init_trans() call failed");
        }
        else
        {
            success = 1;
        }
    }

    if (success)
    {
        g_t_out = t1;
        g_t_in = t2;
        g_t_vanilla = t3;
    }
    else
    {
        trans_delete(t1);
        trans_delete(t2);
        trans_delete(t3);
    }
}

/******************************************************************************/
static void
suite_test_libipm_calls_end(void)
{
    trans_delete(g_t_out);
    trans_delete(g_t_in);
    trans_delete(g_t_vanilla);
}

/******************************************************************************/
void
check_binary_data_eq(const void *actual_data,
                     unsigned int actual_len,
                     const void *expected_data,
                     unsigned int expected_len)
{
    const unsigned char *cp_expected = (const unsigned char *)expected_data;
    const unsigned char *cp_expected_end =
        (const unsigned char *)expected_data + expected_len;
    const unsigned char *cp_actual = (const unsigned char *)actual_data;

    if (expected_len != actual_len)
    {
        ck_abort_msg("Differing lengths. Expected %u, got %u",
                     expected_len, actual_len);
    }

    while (cp_expected < cp_expected_end)
    {
        if (*cp_expected != *cp_actual)
        {
            int byte_pos = cp_expected - (const unsigned char *)expected_data;
            ck_abort_msg("Byte position %d. Expected %02x, got %02x",
                         byte_pos, *cp_expected, *cp_actual);
        }
        ++cp_expected;
        ++cp_actual;
    }
}

/******************************************************************************/

int
does_stream_contain_string(const struct stream *s, const char *str)
{
    int len = g_strlen(str);
    int i;
    if (len > 0 && len <= s->size)
    {
        for (i = 0 ; i <= s->size - len; ++i)
        {
            /* This is not a sophisticated string search. We use a
             * single character test to avoid a function call for
             * every comparison */
            if (str[0] == s->data[i] && g_memcmp(str, &s->data[i], len) == 0)
            {
                return 1;
            }
        }
    }

    return 0;
}


/******************************************************************************/
int main (void)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create (make_suite_test_libipm_send_calls());
    srunner_add_suite (sr, make_suite_test_libipm_recv_calls());

    srunner_set_tap(sr, "-");
    /*
     * Set up console logging */
    struct log_config *lc = log_config_init_for_console(LOG_LEVEL_INFO, NULL);
    log_start_from_param(lc);
    log_config_free(lc);

    /* Initialise modules */
    suite_test_libipm_calls_start();

    srunner_run_all (sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    suite_test_libipm_calls_end();

    log_end();
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
