/*
 * Copyright (C) 1994-2020 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of the PBS Professional ("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * For a copy of the commercial license terms and conditions,
 * go to: (http://www.pbspro.com/UserArea/agreement.html)
 * or contact the Altair Legal Department.
 *
 * Altair’s dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of PBS Pro and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair’s trademarks, including but not limited to "PBS™",
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 * trademark licensing policies.
 *
 */

/**
 * @file	req_gsshandshake.c
 *
 * @brief
 *  Batch request routines for GSS handshake via batch request (TCP).
 */

#include <pbs_config.h>   /* the master config generated by configure */

#if defined(PBS_SECURITY) && (PBS_SECURITY == KRB5)

#include "pbs_gss.h"
#include "pbs_ifl.h"
#include "log.h"
#include "net_connect.h"
#include "server.h"
#include "dis.h"

/* @brief
 *	This should be called on a socket after obtaining client_name via
 *	gss_accept_sec_context. It copies the credid from the client_name
 *	structure to conn_credent.
 *
 * @param[in] s - tcp socket
 *
 * @return	int
 * @retval	0 on success
 * @retval	!= 0 on error
 */
int
gss_set_conn(int s)
{
	char *credid;
	int i;
	int length;
	conn_t *conn;
	pbs_gss_extra_t *gss_extra = NULL;

	/* this is done only once - after the gss handshake */
	if (((gss_extra = (pbs_gss_extra_t *)tcp_get_extra(s)) != NULL) &&
		gss_extra->establishing) {

		conn = get_conn(s);

		free(conn->cn_credid);
		conn->cn_credid = strdup(gss_extra->clientname);
		if (conn->cn_credid == NULL) {
			log_event(PBSEVENT_ERROR | PBSEVENT_FORCE, PBS_EVENTCLASS_SERVER, LOG_ERR, __func__, "malloc failure");
			return -1;
		}

		credid = conn->cn_credid;
		if (!credid) {
			log_err(-1, __func__, "couldn't get client_name");
			return -1;
		}

		for (i = 0; credid[i] != '\0' && i < PBS_MAXUSER; i++) {
			if (credid[i] == '@')
				break;
		}

		length = strlen(credid);
		strncpy(conn->cn_username, credid, i);
		conn->cn_username[i] = '\0';

		strncpy(conn->cn_hostname, credid + i + 1, length - i -1);
		conn->cn_hostname[length - i -1] = '\0';

		log_eventf(PBSEVENT_DEBUG | PBSEVENT_FORCE, PBS_EVENTCLASS_SERVER, LOG_DEBUG, __func__,
			"Entered encrypted communication with %s@%s", conn->cn_username, conn->cn_hostname);

		gss_extra->establishing = 0; /* establishing context is finished - don't do this next time */
	}

	return 0;
}

/* @brief
 *	Process PBS_BATCH_AuthExternal batch request with GSS header and sets
 *	the GSS handshake as started.
 *
 *
 * @param[in] preq - batch request structure
 *
 * @return	int
 * @retval	0 on success
 * @retval	!= 0 on error
 */
int
req_gss_auth(struct batch_request *preq)
{
	pbs_gss_extra_t *gss_extra = NULL;
	int sock;

	sock = preq->rq_conn;

	if ((gss_extra = (pbs_gss_extra_t *)tcp_get_extra(sock)) == NULL){
		gss_extra = pbs_gss_alloc_gss_extra();
		tcp_set_extra(sock, gss_extra);
	}

	gss_extra->establishing = 1; /* the handshake has been initiated */

	return 0;
}
#endif
