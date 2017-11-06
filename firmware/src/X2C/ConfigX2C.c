/* This file is part of X2C. http://www.mechatronic-simulation.org/                                                   */

#include "system_definitions.h"
#include "Target.h"
#include "TableStruct.h"
#include "LNet.h"
#include "Services.h"
#include "BlockServicesX2C.h"
#include "X2C.h"
#include "PortConfigX2C.h"
#include "CommunicationCDC.h"
#include "ConfigX2C.h"

/* Communication state machine */
typedef enum { COMM_PREINIT, COMM_HWINIT, COMM_CONFIGURED, COMM_RUN } tCommStateX2C;
static tCommStateX2C commState = COMM_PREINIT;

/* used by init version info */
const struct {
	uint8 date[11];
	uint8 time[8];
} compilationDate = { __DATE__, __TIME__ };

/* X2C maintenance structure */
volatile tTableStruct TheTableStruct;
volatile tTableStruct *TableStruct = &TheTableStruct;

/* LNet protocol buffersize & node identifier */
#define LNET_BUFFERSIZE ((uint8)255)
#define LNET_NODEID ((uint8)1)
#define KICK_DOG \
    do { \
        ClrWdt(); \
    } while (0)

static tLNet protocol;
static uint8 bufferLNet[LNET_BUFFERSIZE];

/* private prototypes */
static void initVersionInfo(volatile tTableStruct* tblStruct, uint16 appVersion);


/** 
 * @brief Main function.
 * 
 * @return The main function will never return due to the never ending loop.
 */
void X2C_Initialize(void)
{
    /** - initialize "integrated monitor":
     *    - configuration of LNet protocol:
     *      - Node-ID: 1
     *      - Buffer size: 255
     *      .
     *    .
     *  - initialize serial interface
     */
    initTableStruct();
	initCommunication();
    initLNet(&protocol, bufferLNet, LNET_BUFFERSIZE, LNET_NODEID);
	linkCommunication((tProtocol*)&protocol);
    initServiceTable((tProtocol*)&protocol);
    addCoreServices((tProtocol*)&protocol);
    addBlockServices((tProtocol*)&protocol);
    addTableStructProtocol((tProtocol*)&protocol);
    TableStruct->DSPState = PRG_LOADED_STATE;

    initVersionInfo(TableStruct, (uint16)0x0001);   /* software version 1 */
    TableStruct->TFncTable = blockFunctionTable;
    TableStruct->TParamTable = parameterIdTable;

    /** - initialize X2C */
    X2C_Init();
	commState = COMM_HWINIT;
}

void X2C_Communicate(void)
{
    switch (commState)
    {
        case COMM_HWINIT:
            if (initCommunicationHardware())
            {
                commState = COMM_CONFIGURED;
            }
            break;
        case COMM_CONFIGURED:
            if (isCommunicationConfigured())
            {
                commState = COMM_RUN;
            }
            break;
        case COMM_RUN:
            protocol.pCommunicate((tProtocol*)&protocol);
            break;
        default:
            break;
    }
}

void X2C_UpdateModel(void)
{
	readInports();
	X2C_Update();
	writeOutports();
}

/**
 * @brief Routine to set version.
 * 
 * @param tblStruct X2C maintenance structure.
 * @param appVersion Version number.
 */
static void initVersionInfo(volatile tTableStruct* tblStruct, uint16 appVersion)
{
	tblStruct->framePrgVersion = appVersion;
	tblStruct->framePrgCompDateTime = (uint8*)&compilationDate;
}
