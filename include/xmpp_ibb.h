int iq_ibb_open_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata);

int XMPP_IBB_data_process(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata);

void XMPP_IBB_Data_Send(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata);

void XMPP_IBB_Ack_Send(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata);

void XMPP_IBB_Close_Send(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata);
