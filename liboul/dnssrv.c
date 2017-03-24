/**
 * @file dnssrv.c
 */

#include "internal.h"

#include "util.h"

#include <arpa/nameser.h>
#include <resolv.h>

#ifdef HAVE_ARPA_NAMESER_COMPAT_H
#	include <arpa/nameser_compat.h>
#endif

#ifndef T_SRV
#	define T_SRV	33
#endif

#include "dnssrv.h"
#include "eventloop.h"
#include "debug.h"

typedef union {
	HEADER hdr;
	u_char buf[1024];
} queryans;

struct _OulSrvQueryData {
	OulSrvCallback cb;
	gpointer extradata;
	guint handle;
	int fd_in, fd_out;
	pid_t pid;
};

static gint
responsecompare(gconstpointer ar, gconstpointer br)
{
	OulSrvResponse *a = (OulSrvResponse*)ar;
	OulSrvResponse *b = (OulSrvResponse*)br;

	if(a->pref == b->pref) {
		if(a->weight == b->weight)
			return 0;
		if(a->weight < b->weight)
			return -1;
		return 1;
	}
	if(a->pref < b->pref)
		return -1;
	return 1;
}

G_GNUC_NORETURN static void
resolve(int in, int out)
{
	GList *ret = NULL;
	OulSrvResponse *srvres;
	queryans answer;
	int size;
	int qdcount;
	int ancount;
	guchar *end;
	guchar *cp;
	gchar name[256];
	guint16 type, dlen, pref, weight, port;
	gchar query[256];

#ifdef HAVE_SIGNAL_H
	oul_restore_default_signal_handlers();
#endif

	if (read(in, query, 256) <= 0) {
		close(out);
		close(in);
		_exit(0);
	}

	size = res_query( query, C_IN, T_SRV, (u_char*)&answer, sizeof( answer));

	qdcount = ntohs(answer.hdr.qdcount);
	ancount = ntohs(answer.hdr.ancount);

	cp = (guchar*)&answer + sizeof(HEADER);
	end = (guchar*)&answer + size;

	/* skip over unwanted stuff */
	while (qdcount-- > 0 && cp < end) {
		size = dn_expand( (unsigned char*)&answer, end, cp, name, 256);
		if(size < 0) goto end;
		cp += size + QFIXEDSZ;
	}

	while (ancount-- > 0 && cp < end) {
		size = dn_expand((unsigned char*)&answer, end, cp, name, 256);
		if(size < 0)
			goto end;

		cp += size;

		GETSHORT(type,cp);

		/* skip ttl and class since we already know it */
		cp += 6;

		GETSHORT(dlen,cp);

		if (type == T_SRV) {
			GETSHORT(pref,cp);

			GETSHORT(weight,cp);

			GETSHORT(port,cp);

			size = dn_expand( (unsigned char*)&answer, end, cp, name, 256);
			if(size < 0 )
				goto end;

			cp += size;

			srvres = g_new0(OulSrvResponse, 1);
			strcpy(srvres->hostname, name);
			srvres->pref = pref;
			srvres->port = port;
			srvres->weight = weight;

			ret = g_list_insert_sorted(ret, srvres, responsecompare);
		} else {
			cp += dlen;
		}
	}

end:
	size = g_list_length(ret);
	write(out, &size, sizeof(int));
	while (ret != NULL)
	{
		write(out, ret->data, sizeof(OulSrvResponse));
		g_free(ret->data);
		ret = g_list_remove(ret, ret->data);
	}

	close(out);
	close(in);

	_exit(0);
}

static void
resolved(gpointer data, gint source, OulInputCondition cond)
{
	int size;
	OulSrvQueryData *query_data = (OulSrvQueryData*)data;
	OulSrvResponse *res;
	OulSrvResponse *tmp;
	int i;
	OulSrvCallback cb = query_data->cb;
	int status;

	if (read(source, &size, sizeof(int)) == sizeof(int))
	{
		ssize_t red;
		oul_debug_info("dnssrv","found %d SRV entries\n", size);
		tmp = res = g_new0(OulSrvResponse, size);
		for (i = 0; i < size; i++) {
			red = read(source, tmp++, sizeof(OulSrvResponse));
			if (red != sizeof(OulSrvResponse)) {
				oul_debug_error("dnssrv","unable to read srv "
						"response: %s\n", g_strerror(errno));
				size = 0;
				g_free(res);
				res = NULL;
			}
		}
	}
	else
	{
		oul_debug_info("dnssrv","found 0 SRV entries; errno is %i\n", errno);
		size = 0;
		res = NULL;
	}

	cb(res, size, query_data->extradata);
	waitpid(query_data->pid, &status, 0);

	oul_srv_cancel(query_data);
}


OulSrvQueryData *
oul_srv_resolve(const char *protocol, const char *transport, const char *domain, OulSrvCallback cb, gpointer extradata)
{
	char *query;
	OulSrvQueryData *query_data;
	int in[2], out[2];
	int pid;

	query = g_strdup_printf("_%s._%s.%s", protocol, transport, domain);
	oul_debug_info("dnssrv","querying SRV record for %s\n", query);

	if(pipe(in) || pipe(out)) {
		oul_debug_error("dnssrv", "Could not create pipe\n");
		g_free(query);
		cb(NULL, 0, extradata);
		return NULL;
	}

	pid = fork();
	if (pid == -1) {
		oul_debug_error("dnssrv", "Could not create process!\n");
		cb(NULL, 0, extradata);
		g_free(query);
		return NULL;
	}

	/* Child */
	if (pid == 0)
	{
		g_free(query);

		close(out[0]);
		close(in[1]);
		resolve(in[0], out[1]);
		/* resolve() does not return */
	}

	close(out[1]);
	close(in[0]);

	if (write(in[1], query, strlen(query)+1) < 0)
		oul_debug_error("dnssrv", "Could not write to SRV resolver\n");

	query_data = g_new0(OulSrvQueryData, 1);
	query_data->cb = cb;
	query_data->extradata = extradata;
	query_data->pid = pid;
	query_data->fd_out = out[0];
	query_data->fd_in = in[1];
	query_data->handle = oul_input_add(out[0], OUL_INPUT_READ, resolved, query_data);

	g_free(query);

	return query_data;

}

void
oul_srv_cancel(OulSrvQueryData *query_data)
{
	if (query_data->handle > 0)
		oul_input_remove(query_data->handle);

	close(query_data->fd_out);
	close(query_data->fd_in);

	g_free(query_data);
}
