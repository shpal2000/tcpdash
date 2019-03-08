#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#include "jparse.h"

static void ParseConfig (int loop_count) {

    gchar* configTxt;
    gsize configLen;
    GError* configErr;
    gboolean configSts;

    configSts = g_file_get_contents ( "/root/config.txt"
                            , &configTxt
                            , &configLen
                            , &configErr);
    
    if (configSts) {

        JNode* cfgNode;
        JObject* cfgObj;

        for (int i = 0; i < loop_count; i ++) {  
            sleep (0); 
            JGET_ROOT_NODE (configTxt, &cfgNode, &cfgObj);
            // JFREE_ROOT_NODE (cfgNode, cfgObj);

            uint32_t t1, t2,t3, t5;
            uint64_t t4;
            const char* s1;

            if (cfgNode) {
                    JGET_MEMBER_INT (cfgObj, "connPerSec", &t1);
                    JGET_MEMBER_INT (cfgObj, "maxActSessions", &t2);
                    JGET_MEMBER_INT (cfgObj, "maxErrSessions", &t3);
                    JGET_MEMBER_INT (cfgObj, "maxSessions", &t4);

                    JGET_MEMBER_STR (cfgObj, "Id", &s1);

                    JArray* csGroupArr;
                    
                    JGET_MEMBER_ARR (cfgObj, "csGroupArr", &csGroupArr);

                    JObject* csGroup = JGET_ARR_ELEMENT_OBJ (csGroupArr, 0);

                    JGET_MEMBER_INT (csGroup, "clientAddrCount", &t5);

                    JFREE_ROOT_NODE (cfgNode, cfgObj);
            } else {

            }
        }

        g_free (configTxt);
    } else {

    }
}

int main(int argc, char** argv) {
    ParseConfig (1000000);
}