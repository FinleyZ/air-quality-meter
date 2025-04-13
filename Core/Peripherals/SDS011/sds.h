void SDS_getData(void);
void SDS_setSleepMode(void);
void SDS_setWorkMode(void);

extern uint8_t sds_response_ready;
extern uint8_t sds_command_waiting;
extern uint8_t SDS_receivedData[10];

void SDS_setActiveReportingMode(void);
void SDS_setQueryReportingMode(void);
void SDS_queryCurrentWorkMode(void);
void SDS_queryData(void);

void SDS_logReceivedData(void);
void SDS_logPMData(void);