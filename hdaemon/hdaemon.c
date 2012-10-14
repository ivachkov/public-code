/**
 *
 *
 *
**/

#include <netinet/in.h>
#include <linux/wireless.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>
#include <err.h>
#include <syslog.h>
#include <fcntl.h>

#include <ewget.h>

#define FALSE 0
#define TRUE 1

int zIsDebug = FALSE;

#define LOG		syslog

#define PROC_UPTIME		 "/proc/uptime"
#define PROC_MEMINFO		 "/proc/meminfo"
#define PROC_LOADAVG		 "/proc/loadavg"

#define FALSE			0
#define TRUE			1

#define LEDS1ON			4
#define LEDS2ON			3
#define LEDS3ON			2
#define LEDSFLASH		1

#define SECS_IN_DAY		86400
#define SECS_IN_HOUR		3600
#define SECS_IN_MIN		60

#define MESH_IFACE		"wlan2"
#define WAN_IFACE				"eth0"

#define LEDFILE			"/tmp/ledstatus.txt"

#define TWO_GHZ_CHANNEL     11
#define FIVE_GHZ_CHANNEL    161

#define TWO_GHZ_N_HWMODE    "11ng"
#define FIVE_GHZ_N_HWMODE   "11na"

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE	  ((int)(size * sizeof (struct ifreq)))

typedef enum _role {UNKNOWN, REPEATER, GATEWAY } role;

int set_var_from_shell(char* output, int output_len, const char* cmd, ...);


int gw_hex_to_dec(char* gw, int max_len)
{
	char buf[2];
	int digit[4];
	int i;

	
	for(i=0; i<4; i++) {
		strncpy(buf, (gw+i*2), 2);
		sscanf(buf, "%x", &digit[i]);
	}

	snprintf(gw, max_len, "%d.%d.%d.%d", digit[0], digit[1], digit[2], digit[3]);

	return 0;
}

role role_gw(char* gw, int gw_len)
{
	FILE* fp;
	char buf[1024];
	char dest[128];
	char iface[12];
	int res;
	role ret_val = UNKNOWN;

	
	/* read routing table */
	if((fp = fopen("/proc/net/route", "r")) == NULL) {
		return ret_val;
	}

	while(fgets(buf, sizeof(buf), fp)) {
		/* get destination */
		if((res = get_substring(dest, sizeof(dest), buf, 2)) != 0) {
			fclose(fp);
			return ret_val;
		}

		/* if record is not for default gw - skip it */ 
		if( (strcmp(dest, "00000000")) != 0)
			continue;
	
		/* if it is - get interface */
		if((res = get_substring(iface, sizeof(iface), buf, 1)) != 0) {
			fclose(fp);
			return ret_val;
		}

		if((strcmp(iface, "wlan2")) == 0) {
			ret_val = REPEATER;

			/* and get default gw */
			if((res = get_substring(gw, gw_len, buf, 3)) !=0) {
				fclose(fp);
				return ret_val;
			}

			gw_hex_to_dec(gw, gw_len);
			break;
		} else if((strcmp(iface, "eth0")) == 0) {
			ret_val = GATEWAY;
		}

		/* and get default gw */
		if((res = get_substring(gw, gw_len, buf, 3)) !=0) {
			fclose(fp);
			return ret_val;
		}
		gw_hex_to_dec(gw, gw_len);
	}

	fclose(fp);

	return ret_val;
}

int hop_rtt(char *gw, int *hops, char *rtt) 
{
	char **ap, *args[10], *line, *l = NULL;
	char command[1024];
	FILE *f = NULL;

	memset(command, 0, 1024);

	snprintf(command, 1024, "traceroute -n %s | grep -v traceroute | tail -1 | awk '{print $1, $3 }'", gw);

	if ((f = popen(command, "r")) == NULL)
		return -1;

	if ((line = malloc(255)) == NULL) {
		pclose(f);
		return -1;
	}
	l = line;

	memset(line, 0, 255);

	fgets(line, 255, f);

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;
				
	if (strlen(args[0]) > 0)
		*hops = atoi(args[0]);

	if (strlen(args[1]) > 0)
		strcpy(rtt, args[1]);
		//*rtt = strtof(args[1], NULL);
			
	free(l);
	pclose(f);
	return 0;
}

int count_hops(char* out, int out_len)
{
	role our_role;
	char gw[16];
	
	char rtt[128];
	int hops = 0;
	
	
	our_role = role_gw(gw, sizeof(gw));

	if(our_role == GATEWAY) {
		debugwrite("Gateway Mode\n");
		strcpy(rtt, "0");
		hops = 1;
	} else if(our_role == REPEATER) {
		debugwrite("Repeater Mode\n");
		hop_rtt(gw, &hops, rtt);
	}
	
	snprintf(out, out_len, "&routes=%s&hops=%d&RTT=%s", gw, hops, rtt);

	return 0;
}

int get_ip(char * ifname, char* out, int out_len) {
	struct ifreq		*ifr;
	struct ifreq		ifrr;
	struct sockaddr_in	sa;
	struct sockaddr ifaddr;
	int			sockfd;

	
	if((sockfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1) {
		debugwrite("Socket for DGRAM cannot be created\n");
		return -1;
	}

	ifr = &ifrr;

	ifrr.ifr_addr.sa_family = AF_INET;

	strncpy(ifrr.ifr_name, ifname, sizeof(ifrr.ifr_name));

	if (ioctl(sockfd, SIOCGIFADDR, ifr) < 0) {
		debugwrite("No %s interface.\n", ifname);
		close(sockfd);
		return -1;
	}

	ifaddr = ifrr.ifr_addr;
	snprintf(out, out_len, "&ip=%s", inet_ntoa(inaddrr(ifr_addr.sa_data)) );

	close(sockfd);
	return 0;

}

int get_wan(char * ifname, char* out, int out_len) {
	struct ifreq			*ifr;
	struct ifreq			ifrr;
	struct sockaddr_in		sa;
	struct sockaddr 		ifaddr;
	int				sockfd;

	
	if((sockfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1) {
		debugwrite("Socket for DGRAM cannot be created\n");
		return -1;
	}

	ifr = &ifrr;

	ifrr.ifr_addr.sa_family = AF_INET;

	strncpy(ifrr.ifr_name, ifname, sizeof(ifrr.ifr_name));

	if (ioctl(sockfd, SIOCGIFADDR, ifr) < 0) {
		debugwrite("No %s interface.\n", ifname);
		close(sockfd);
		return -1;
	}

	ifaddr = ifrr.ifr_addr;
	snprintf(out, out_len, "&wan=%s", inet_ntoa(inaddrr(ifr_addr.sa_data)) );

	close(sockfd);
	return 0;
}

int get_version(char* version)
{
	FILE *fp;
	char buf[1024];
	char *pos;
	int i;

	
	fp = fopen("/etc/version", "r");
	if( fp ==  NULL) {
		return 1;
	}

	if(!fgets(buf, sizeof(buf), fp)) {
		fclose(fp);
		return 2;
	}

	i = strlen(buf) - 1;
	while(buf[i] == '\n' || buf [i] == '\r') {
		buf[i] = '\0';
		i--;
	}

	sprintf(version, "&ver=%s", buf);
	fclose(fp);
	return 0;
}

int get_substring(char *out, int out_size, const char *data, int pos)
{
	int len;
	int i;
	const char *start = NULL;
	const char *end = NULL;
	int count;
	int in_string;
	int string_found;

	
	len = strlen(data);
	count = 0;
	in_string = 0;
	string_found = 0;

	for (i = 0; i < len; i++) {
		if (in_string == 0) {
			/* First check if we have EOF */
			if (data[i] == '\0') {
				return 1;	/* substring doesn't exist in data */
			}
			/* We are looking for the start of a string */
			if (data[i] == ' ' || data[i] == '\t' || data[i] == '\n') {
				/* We are in "white spaces" between strings - continue */
				continue;
			} else {
				/* We found a string - save the start */
				in_string = 1;
				start = &data[i];
			}
		} else {	/* in_string == 1 */
			/* We are looking for the end of string */
			if (data[i] == ' ' || data[i] == '\t'
				|| data[i] == '\n' || data[i] == '\0') {
				/* We found the end - save it */
				in_string = 0;
				end = &data[i];
				count++;
				/* Check if this is the desired substring */
				if (count == pos) {
					string_found = 1;
					break;
				} else if (data[i] == '\0') {
					/* This was the last substring but it is not ours - return */
					return 1;	/* substring doesn't exist in data */
				}
			} else {
				/* We are still in string - continue */
				continue;
			}
		}
	}

	if (string_found == 1) {
		if (end - start + 1 > out_size) {
			return 2;	/* buf too short */
		}

		if (start && end) {
			strncpy(out, start, end - start);
			out[end - start] = '\0';
			return 0;
		}
	} else {
		return 1;	/* substring doesn't exist in data */
	}

	return 20;
}

/* Get system uptime */
int get_uptime(char *out, int out_len)
{
	FILE *fp;
	char buf[1024];
	char uptime_str[256];
	int count;
	int uptime_secs;
	int days;
	int hours;
	int minutes;
	div_t res;
		

	if ((fp = fopen(PROC_UPTIME, "r")) == NULL) {
		return 1;
	}

	count = fread(buf, sizeof(buf) - 1, 1, fp);

	fclose(fp);

	get_substring(uptime_str, sizeof(uptime_str), buf, 1);
	uptime_secs = atoi(uptime_str);

	res = div(uptime_secs, SECS_IN_DAY);
	days = res.quot;
	uptime_secs -= days * SECS_IN_DAY;

	res = div(uptime_secs, SECS_IN_HOUR);
	hours = res.quot;
	uptime_secs -= hours * SECS_IN_HOUR;

	res = div(uptime_secs, SECS_IN_MIN);
	minutes = res.quot;
	uptime_secs -= minutes * SECS_IN_MIN;

	snprintf(out, out_len - 1, "&uptime=%0dd:%0dh:%02dm", days, hours, minutes);

	return 0;
}

/* Get Free Memory */
int get_free_mem(char* out, int out_len)
{
	FILE* fp;
	int retval = 0;
	char line_buf[1024];
	char out_buf[1024];

	char field[] = "MemFree:";

	if ((fp = fopen(PROC_MEMINFO, "r")) == NULL)
		return 1;

	while(fgets(line_buf, sizeof(line_buf), fp)) {
		if(strncmp(line_buf, field, strlen(field)) == 0) {
			if(get_substring(out_buf, sizeof(out_buf), line_buf, 2) == 0) {
				retval = 0;
				snprintf(out, out_len, "&memfree=%s", out_buf);
			}
			break;
		}
	}

	fclose (fp);

	return retval;
}

/* Get System Load Average */
int get_load(char *load, int load_len)
{
	char **ap, *args[10], *line, *l = NULL;
	FILE *f = NULL;

	if ((f = fopen(PROC_LOADAVG, "r")) == NULL)
		return -1;

	if ((line = malloc(255)) == NULL) {
		fclose(f);
		return -1;
	}

	l = line;
	memset(line, 0, 255);
	fgets(line, 255, f);

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;

	if (strlen(args[0]) > 0 && strlen(args[1]) > 0 && strlen(args[2]) > 0)
		snprintf(load, load_len, "&load=%s,%s,%s", args[0], args[1], args[2]);

	free(l);
	fclose(f);

	return 0;
}

int get_ssid(const char* ifname, char* ssidname, int ssid_len)
{
//                int s, retval;
//                struct iwreq iwr;

        const char* keyword = "ESSID: \"";

        FILE* fp;
        char* start_pos;
        char* end_pos;
        char buf[256];
        char shell_cmd[128];
        int retval = 100;


        memset(buf, '\0', sizeof(buf));

        memset(shell_cmd, '\0', sizeof(shell_cmd));

        /* prepare the command */

        snprintf(shell_cmd, sizeof(shell_cmd)-1, "iwinfo %s info", ifname);

        /* run cmd */

        if((fp = popen(shell_cmd, "r")) == NULL) {
                return 1;
        }

        /* parse output - the line we are looking for is something like: 
         * wlan0     Type: nl80211  ESSID: "xmff"
         * The field of interest is ESSID - it's saved in keyword const
         */

        while(fgets(buf, sizeof(buf)-1, fp)) {
                /* find the start of the keyword in the line */
                if((start_pos = strstr(buf, keyword)) == NULL)
                        continue;

                /* moves the pointer to the actual SSID */

                start_pos += strlen(keyword);
                /* find the end - closing " */
                if((end_pos = strchr(start_pos, '"')) == NULL) {
                        retval = 1;
                        break;

                }

                /* check if our buffer is big enough */

                if((end_pos - start_pos) > ssid_len) {
                        retval = 2;
                        break;
                }

                /* prepare output */
                memset(ssidname, '\0', ssid_len);

                /* now we copy SSID without the trailing " */
                strncpy(ssidname, start_pos, (end_pos - start_pos));

                /* we are ready */
                retval = 0;
                break;

        }

        pclose(fp);

        return 0;
}

int get_channel(const char* ifname, char* out, int out_len)
{
        const char* keyword = "Channel: ";

        FILE* fp;
        char* start_pos;
        char* end_pos;
        char buf[256];
        char shell_cmd[128];
        int retval = 100;

        memset(buf, '\0', sizeof(buf));
        memset(shell_cmd, '\0', sizeof(shell_cmd));

        if (ifname == NULL || out == NULL || out_len == 0) {
                return -1;
        }

        /* prepare the command */
        snprintf(shell_cmd, sizeof(shell_cmd)-1, "iwinfo %s info", ifname);

        /* run cmd */
        if((fp = popen(shell_cmd, "r")) == NULL) {
                return 1;
        }

        /* parse output - the line we are looking for is something like: 
         * Mode: Client  Channel: 13 (2.472 GHz)
         * The field of interest is Channel - it's saved in keyword const
         */
        while(fgets(buf, sizeof(buf)-1, fp)) {
                /* find the start of the keyword in the line */
                if((start_pos = strstr(buf, keyword)) == NULL)
                        continue;
                /* moves the pointer to the actual Channel */
                start_pos += strlen(keyword);
                /* find the end - a space */
                if((end_pos = strchr(start_pos, ' ')) == NULL) {
                        retval = 1;
                        break;
                }
                /* check if our buffer is big enough */
                if((end_pos - start_pos) > out_len) {
                        retval = 2;
                        break;
                }

                /* prepare output */
                memset(out, '\0', out_len);

                /* now we copy channel without the trailing " */
                strncpy(out, start_pos, (end_pos - start_pos));
  
                /* we are ready */
                retval = 0;
                break;
        }

        pclose(fp);
        return retval;
}

int get_iface_rssi(const char* iface, const char* mac, char* rssi, int rssi_len)
{
        FILE* fp = NULL;
        char line_buf[1024];
        char shell_cmd[256];
        char* pos_start = NULL;
        char* pos_end = NULL;
        int mac_len = 0;
        int res;
        int ret = 100;
        mac_len = strlen(mac);
        snprintf(shell_cmd, sizeof(shell_cmd)-1, "iwinfo %s assoclist", iface);
        if ((fp = popen(shell_cmd, "r")) == NULL)
                return -1;
        memset(line_buf, '\0', sizeof(line_buf));
        /* we parse something like this:
         * root@OpenWrt:~# iwinfo wlan1 assoclist
         * 00:21:5D:96:F1:34  -51 dBm
         */
        while(fgets(line_buf, sizeof(line_buf)-1, fp)) {
                if((res = strncasecmp(mac, line_buf, mac_len)) != 0)
                        continue;
                pos_start = line_buf + mac_len;
                /* skip the spaces after mac */
                while(*pos_start == ' ')
                        pos_start++;
                if((pos_end = strstr(line_buf, " dBm")) == NULL) {
                        ret = -1;
                        goto end;
                }
                if(pos_end - pos_start + 1 > rssi_len) {
                        ret = -2;
                        goto end;
                }
                memset(rssi, '\0', rssi_len);
                strncpy(rssi, pos_start, pos_end - pos_start);
                ret = 0;
        }
end:
        pclose(fp);
        return ret;
}

int get_rssi(const char *ip, char *rssi, int rssi_len)
{
                char **ap, *args[10], *line, *l = NULL;
                char mac[64];
                FILE* f;
                int res;

                memset(mac, 0, 64);

                if (ip == NULL || rssi == NULL)
                        return -1;

                if ((f = fopen("/proc/net/arp", "r")) == NULL)
                        return -1;

                while (!feof(f)) {
                        if ((line = malloc(255)) == NULL) {
                                fclose(f);
                                return -1;
                        }
                        l = line;

                        memset(line, 0, 255);
                        fgets(line, 255, f);

                        if (strlen(line) == 0) {
                                free(line);
                                break;
                        }

                        for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
                                if (**ap != '\0')
                                        if (++ap >= &args[10])
                                                break;

                                if (strncmp(args[0], ip, strlen(args[0])) == 0)
                                        if (strlen(args[3]) > 0) {
                                                memcpy(mac, args[3], 63);
                                                free(line);
                                                break;
                                        }

                                free(l); l = NULL;
                }
                fclose(f);

                if((res = get_iface_rssi("wlan2", mac, rssi, rssi_len)) != 0) {
                        return -1;

                }

                return 0;
}

int get_rx_tx_costs(char *neighbor, char *rx, char *tx, int len)
{
	char **ap, *args[20], *line = NULL, *l = NULL;
	char command[1024];
	char fmt_mac[32];
	char *tmp = NULL;
	FILE *f = NULL;
	char i = 0;

	memset(fmt_mac, 0, sizeof(fmt_mac));

	if (rx == NULL || tx == NULL || neighbor == NULL)
		return -1;

	/* Format last three octets of the MAC in the form AA:BBCC */
	if ((l = line = malloc(64)) == NULL)
		return -1;
	memset(line, 0, 64);
	memcpy(line, neighbor, 63);

	for (ap = args; (*ap = strsep(&line, " :\t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[20])
				break;

	if (strlen(args[3]) > 0 && strlen(args[4]) > 0 && strlen(args[5]) > 0)
		snprintf(fmt_mac, sizeof(fmt_mac) - 1, "%s:%s%s", args[3], args[4], args[5]);

	free(l);

	for (i = 0; i < strlen(fmt_mac); i++)
		fmt_mac[i] = tolower(fmt_mac[i]);

	memset(command, 0, 1024);
	snprintf(command, 1024, "echo  | nc ::1 33123 | grep '^add neighbour ' | grep wlan2 | grep %s", fmt_mac);

	if ((f = popen(command, "r")) == NULL)
		return -1;

	if ((l = line = malloc(255)) == NULL) { 
		pclose(f);
		return -1;
	}		
	memset(line, 0, 255);
	fgets(line, 255, f);

	if (strlen(line) > 0) {
		for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
			if (**ap != '\0')
				if (++ap >= &args[20])
					break;	

		if (strlen(args[10]) > 0 && strlen(args[12]) > 0) {
			memcpy(rx, args[10], len);
			memcpy(tx, args[12], len);
		}		
	}

	free(l);				
	pclose(f);
	return 0;
}

int set_var_from_shell(char* output, int output_len, const char* cmd, ...)
{

	FILE* fp;
	va_list params;
	char buffer[2048];
	int res;
	int i;

	LOG(LOG_DEBUG, "In set_var_from_shell()");

	va_start(params, cmd);

	res = vsnprintf(buffer, sizeof(buffer), cmd, params);

	va_end(params);

	fp = popen(buffer, "r");

	if (fp == NULL) {


		return(-1);

	}

	if (fgets(output, output_len, fp) == NULL) {


		pclose(fp);

		return(-2);

	}

	/* clear the trailing '\n' characters */

	for(i=strlen(output)-1; i>=0; i--) {

		if(output[i] == '\n') {

			output[i] = '\0';

		}

		else {

			break;

		}

	}

	pclose(fp);

	return 0;

}

int zLedState = LEDSFLASH;
int zAreWeOnline = FALSE;

int zIsDaemon = FALSE;
int zIsForeground = FALSE;

char zSSID[128] = "OptimWIFI";
int zFreq = 0; 
int zChannel = 0;
char zServer[32] = "meshconnect.optimlabs.com";
char zURL[32] = "checkin-batman.php";
int loopcounts = 0;
int zFIRST = 0;
int mesh_prefix= 5;

int main(int argc, char **argv)
{
	int i;
	char myarg[128];
	pid_t pid;
	int ch = 0;
	int retval = 0;
	char lcounts[16];
	unsigned int lc = 0;

	memset(lcounts, 0, 16);

	/* Parse command line parameters */
	while ((ch = getopt(argc, argv, "df")) != -1) {
		switch (ch) {
		case 'd':
			zIsDaemon = TRUE;
			break;
		case 'f':
			zIsForeground = TRUE;
			break;
		default:
			show_usage();
			exit(-1);
		}
	}

	if (!zIsDaemon && !zIsForeground) {
		show_usage();
		exit(-1);
	}

	if (zIsForeground) {
		debugwrite("Starting in foreground mode");
	}

	if (zIsDaemon) {
		pid = fork();
	} else {
		// In foreground mode
		debugwrite("In foreground mode so not daemonizing");
		pid = 0;
	}

	debugwrite("checking if first time we are run");
		
	if (isFirstRun()) {
		debugwrite("this IS the first time we've been run");
		//
		// This is the first run!
		//
		doFirstRunWork();
		// return 0;
	} else {
		debugwrite("we've been run before - so exiting");
		// return 0;
	}

	if (0 == pid) {
		//
		// This is the child (the daemon) - or we are in foreground mode!
		//
		debugwrite("Loading our radio configuration");
		loadRadioConfiguration();
		debugwrite("Starting our work now...");
		while (TRUE) {
			// Get update interval
			if (set_var_from_shell(lcounts, sizeof(lcounts), "/sbin/uci get general.services.updt_time") != 0) {
				debugwrite("uci get general.services.updt_time ERROR");
				lc = 6;	
			} else {
				lc = atoi(lcounts);
				if (lc == 0) 
					lc = 6;
				memset(lcounts, 0, 16);
			}

			// Check our signal strength (for our LEDs)
			debugwrite("checking LED status");
			updateLEDs();

			loopcounts++;
			if (lc  <= loopcounts) {
				//update conf
				loadRadioConfiguration();
				// Send data to our central server
				debugwrite("phoning home");
				phonehome();

				// Reset loopcounts
				loopcounts = 0;
			}

			sleep(10);		// Sleep 6 seconds
		}
	}
}

int isFirstRun() 
{
	int res;
	char first_boot[5];
	int retval = FALSE;

        memset(first_boot, 0, sizeof(first_boot));
		
	if(set_var_from_shell(first_boot, sizeof(first_boot), "uci get general.services.firstboot") != 0) {
                strcpy(first_boot, "1");
	}

	if(strcmp(first_boot, "0") == 0) {
		system("/sbin/uci set general.services.firstboot=1");
                system("/sbin/uci commit general");

		retval = TRUE;
	} else {
		debugwrite("isFirstRun> This is the first boot ");
		retval = FALSE;
	}

	return retval;
}

int doFirstRunWork()
{
        char mymac[20]; // 17 spaces for our MAC address
        char myathip[20];               // IP address for wlan0 MESH I/F
	char myethip[20];        // IP address for my tunnel I/F
	char mypublicip[20];        // IP address for my tunnel I/F
	char myprivateip[20];        // IP address for my tunnel I/F
        char mytunip[20];               // IP address for my tunnel I/F
        char oct_a[64]; /* work var for ip_base */
        char oct_b[64]; /* work var for ip_base */
        char oct_c[64];
        int num_a;
        int num_b;
        int num_c;
        int fd;
        char mesh_prefix[64];
        char ip_mesh[64];
        char system_command[100];

        debugwrite("isFirstRun> getMyMac ");
        getMyMac(mymac);
        debugwrite("isFirstRun> getMyMac Done ");
        debugwrite("isFirstRun> Start IP Calc ");

        /* Calculate IP mesh iface based on MAC address in the subnet 5.x.x.x */
        oct_a[0] = '0';
        oct_a[1] = 'x';
        oct_a[2] = *(mymac+9);
        oct_a[3] = *(mymac+10);
        oct_a[4] = '\0';
        sscanf(oct_a, "%x", &num_a);
        oct_b[0] = '0';
        oct_b[1] = 'x';
        oct_b[2] = *(mymac+12);
        oct_b[3] = *(mymac+13);
        oct_b[4] = '\0';
        sscanf(oct_b, "%x", &num_b);
        oct_c[0] = '0';
        oct_c[1] = 'x';
        oct_c[2] = *(mymac+15);
        oct_c[3] = *(mymac+16);
        oct_c[4] = '\0';
        sscanf(oct_c, "%x", &num_c);
	// 5.X address not needed for wing - babel/olsr need 5.x
        snprintf(myathip, sizeof(myathip), "5.%d.%d.%d", num_a, num_b, num_c);
        snprintf(myethip, sizeof(myethip), "10.%d.%d.193", num_b, num_c);
        snprintf(mypublicip, sizeof(myathip), "10.%d.%d.1", num_b, num_c);
        snprintf(myprivateip, sizeof(myathip), "10.%d.%d.129", num_b, num_c);

        debugwrite("isFirstRun> Calculate IP Done ");

	// 5.X address not needed for wing - babel/olsr need 5.x
        // sprintf(system_command, "/sbin/uci set network.mesh.ipaddr=%s", myathip);
        //  system(system_command);

	sprintf(system_command, "/sbin/uci set network.ssid1.ipaddr=%s", myathip);
        system(system_command);

        sprintf(system_command, "/sbin/uci set network.ssid2.ipaddr=%s", myathip);
        system(system_command);

        sprintf(system_command, "/sbin/uci set network.lan.ipaddr=%s", myathip);
        system(system_command);

        sprintf(system_command, "/sbin/uci commit network");
        system(system_command);
        
	sprintf(system_command, "/usr/sbin/apply_config -a");
        system(system_command);

	getRadioFreq(); 
 
	if( zFreq == 0 || zChannel == 0 ) { 
	debugwrite("Error getting Frequency and Channel data"); 
	} 
	else { 
	sprintf(system_command, "/sbin/uci set wireless.radio0.channel=%d", zChannel); 
        system(system_command);
	sprintf(system_command, "/sbin/uci commit wireless"); 
        system(system_command);

	sprintf(system_command, "/sbin/uci set general.services.frequency=%d", zFreq); 
        system(system_command);
	sprintf(system_command, "/sbin/uci commit general"); 
        system(system_command);

	sprintf(system_command, "/sbin/uci set system.upgrade.repository=%d/firmware/%d/", zServer, zFreq); 
        system(system_command);
	sprintf(system_command, "/sbin/uci commit system"); 
        system(system_command);
	} 
 
	flagFirstRunAsCompleted();

        debugwrite("isFirstRun> Flag first run completed ");
    	return 0;
}

int getRadioFreq() 
{ 
	char tempChan[2]; 
	set_var_from_shell(tempChan, sizeof(tempChan), "iwinfo radio0 freqlist | head -1 | cut -c3"); 
 
	if ( strcmp (tempChan, "2") == 0 ) { 
	zChannel = TWO_GHZ_CHANNEL; 
	zFreq = atoi(tempChan); 
	} 
 
	if ( strcmp (tempChan, "5") == 0 ) { 
	zChannel = FIVE_GHZ_CHANNEL; 
	zFreq = atoi(tempChan); 
	} 
}

int flagFirstRunAsCompleted()
{
	FILE *outfile;

	if (outfile = fopen("/etc/first-run-completed.txt", "wt")) {
		fprintf(outfile, "First Run has been completed.\n");
		fclose(outfile);
	}

	return 0;
}

int show_usage()
{
	printf("Usage:\n\nhdaemon -f -d \n\n\t-f = Foreground\n\t-d = Daemon\n");

	return 0;
}

int loadRadioConfiguration()
{
	char buf[128];

	/* set zSSID from wlan2 */
	if(get_ssid("wlan0", zSSID, sizeof(zSSID)) != 0) {
		printf("loadRadioConfiguration(): Error getting ssid for wlan0\n");
		return 1;
	}

	if(set_var_from_shell(zURL, sizeof(zURL), "uci get general.services.checker") != 0) {
		printf("loadRadioConfiguration(): error getting checkout URL\n");
		return 2;
	}
	
	if(set_var_from_shell(zServer, sizeof(zServer), "uci get general.services.updt_srv") != 0) {
		printf("loadRadioConfiguration(): error getting server address\n");
		return 3;
	}
	
	if(get_channel("wlan2", buf, sizeof(buf) != 0)) {
		printf("loadRadioConfiguration(): error getting channel\n");
		return 3;
	}

	zChannel = atoi(buf);

	if(set_var_from_shell(buf, sizeof(buf), "uci get general.services.updt_time") != 0) {
		printf("loadRadioConfiguration(): error getting server address\n");
		return 3;
	}
	loopcounts = atoi(buf);

	return 0;
}

int updateLEDs()
{
	int newledstate = 0;
	int oldAreWeOnline = zAreWeOnline;

	
	if (isCableInternetFeed()) {
		debugwrite("Cable internet feed - turning ALL LEDS on");
		newledstate = LEDS3ON;
		zAreWeOnline = TRUE;
	} else {
		debugwrite("NOT a cable connection so LEDS are based on Wireless stuff");
		// 1: Do we have any OLSR neighbors at all ?
		if (isAnyMeshNeighbors()) {
			// 2: Do we have a DEFAULT Route?
			if (isMeshDefaultRoute()) {
				debugwrite("We are Repeater - Initialiaze eth0:");
				system("/sbin/uci set general.services.repeater=1");

				system("/sbin/uci set network.lan=interface");
				system("/sbin/uci set network.lan.proto=static");
				system("/sbin/uci set network.lan.ipaddr=192.168.101.254");
				system("/sbin/uci set network.lan.netmask=255.255.255.0");
				system("/sbin/uci set network.lan.dns=208.67.222.222");
				system("/sbin/uci commit network");
				system("/etc/init.d/network restart");
				system("/etc/init.d/dnsmasq restart");
				debugwrite("Eth0 Initialiazed");
//	} else {

				if (isMeshDefaultRouteGoodSignal()) {
					newledstate = LEDS3ON;
				} else {
					newledstate = LEDS2ON;
				}
				zAreWeOnline = TRUE;
			} else {
				newledstate = LEDS1ON;
				zAreWeOnline = FALSE;
			}
		} else {
			newledstate = LEDSFLASH;
			zAreWeOnline = FALSE;
		}
	}

	// Write out our state file (to tmp memory filesystem)
	if (newledstate != zLedState) {
		zLedState = newledstate;
		updateLedState();
	}
	//
	// If our online state has changed we need to update our SSID to reflect this
	//
	return 0;
}

int updateLedState()
{
	FILE *outfile;

	
	if (outfile = fopen(LEDFILE, "wt")) {
		switch (zLedState) {
		debugwrite("updateLEDState");
		case LEDS1ON:
			fprintf(outfile, "LED1ON\n");
			break;
		case LEDS2ON:
			fprintf(outfile, "LED2ON\n");
			break;
		case LEDS3ON:
			fprintf(outfile, "LED3ON\n");
			break;
		case LEDSFLASH:
			fprintf(outfile, "LEDFLASH\n");
			break;
		default:
			fprintf(outfile, "LEDFLASH\n");
			break;
		}

		fclose(outfile);
	}
	return 0;
}

int isMeshDefaultRoute()
{
	char buf[1024];
	FILE *fp;
	int isDefault;

	
	isDefault = FALSE;
	debugwrite("isMeshDefaultRoute");

	if ((fp = popen("netstat -rn", "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		if (doesStartWith(buf, "0.0.0.0") &&
			doesContain(buf, "wlan2")) {
			isDefault = TRUE;
			break;
		}
	}
	pclose(fp);

	if (isDefault) {
		return TRUE;
	}

	return FALSE;
}

int isEthernetDefaultRoute()
{
	char buf[1024];
	FILE *fp;
	int isDefault;

	
	isDefault = FALSE;

	if ((fp = popen("netstat -rn", "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		if (doesStartWith(buf, "0.0.0.0") &&
			doesContain(buf, "eth0")) {
			isDefault = TRUE;
			break;
		}
	}
	pclose(fp);

	return isDefault;
}

int isMeshDefaultRouteGoodSignal()
{
	char drip[128];
	char drmac[128];
	int isSignalGood;

	
	getMeshDefaultRouteIp(drip, sizeof(drip));
	getMeshRouteMacForIp(drip, drmac);
	isSignalGood = isWirelessSignalGood(drmac);

	return isSignalGood;
}

int isWirelessSignalGood(char *drmac)
{
	char buf[8];
	int signal;
	int res;

	makeMacLowercase(drmac);

	if((res = get_iface_rssi("wlan2", drmac, buf, sizeof(buf))) != 0) {
		return FALSE;
	}

	signal = atoi(buf);
	
	if (signal > -78)
		return TRUE;
	else
		return FALSE;
}

int getMeshDefaultRouteIp(char *drip, int drip_len)
{
	char buf[1024];
	FILE *fp;
	char *tokarray[50];
	char token[128];
	int tokloop;
	char gw_buf[64];

	
	// First put some default value into our return IP var
	strcpy(drip, "0.0.0.0");

	if ((fp = popen("netstat -rn", "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		if (doesStartWith(buf, "0.0.0.0")) {
			get_substring(drip, drip_len, buf, 2);
		}
	}
	pclose(fp);
	
	return 0;
}

int getMeshRouteMacForIp(char *drip, char *drmac)
{
	char buf[1024];
	char command[128];
	FILE *fp;
	char *tokarray[50];
	char token[128];
	int tokloop;

	
	// Copy a default value into our return mac variable
	strcpy(drmac, "00:00:00:00:00:00");

	//sprintf(command, "arp -i wlan2 | grep %s", drip);
	sprintf(command, "cat /proc/net/arp | grep %s", drip);

	if ((fp = popen(command, "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		if (doesStartWith(buf, drip)) {
			//
			// This is the default route via a MESH neighbor
			//
			tokarray[0] = strtok(buf, " ");
			if (NULL == tokarray[0]) {
				// No tokens at all!
			} else {
				for (tokloop = 1; tokloop < 50; tokloop++) {
					tokarray[tokloop] = strtok(NULL, " ");
					if (NULL == tokarray[tokloop]) {
						// End of tokens
						break;
					}
					if (3 == tokloop) {
						strcpy(drmac, tokarray[tokloop]);
						break;
					}
				}
			}
		}
	}
	pclose(fp);
	return 0;
}

int isAnyMeshNeighbors()
{
	char buf[1024];
	FILE *fp;

	int isAnyNeighbors = FALSE;
	int isAtNeighborDataStart = FALSE;

	
	if ((fp = popen("echo  | nc ::1 33123 | grep '^add neighbour'", "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		if (doesStartWith(buf, "add neighbour")) {
			isAtNeighborDataStart = TRUE;
		} else if (doesStartWith(buf, "add neighbour")) {
			// Neighbor data end - quit looking here
			break;
		} else {
			if (isAtNeighborDataStart) {
				if (doesStartWith(buf, "add neighbour")) {
					isAnyNeighbors = TRUE;
					break;
				}
			}
		}
	}
	pclose(fp);

	if (isAnyNeighbors) {
		return TRUE;
	}

	return FALSE;
}

int phonehome()
{
	char extras[10240]; // 10K for extra data to pass to our HO
	char mymac[20];		// 17 spaces for MAC address
	char mymac2[20];	// My 2nd Mac (Virtual Mac / Virtual Wireless athX)
	char load[32];		// temporary load storage		
	char uptime[64];
	char freemem[64];
	char version[64];
	char ssid1[64];
	char ssid2[64];
	char gw_ip[64];
	char wan_ip[64];
	char ch1[8];
	char rssi[64];
	char wan_dhcp[8];

	char *pos;		/* current position in extras */

	
	/* init the bufs */
	memset(extras, 0, sizeof(extras));
	memset(mymac, 0, sizeof(mymac));
	memset(mymac2, 0, sizeof(mymac2));
	memset(load, 0, sizeof(load));
	memset(uptime, 0, 64);
	memset(freemem, 0, 64);
	memset(version, 0, 64);
	memset(ssid1, 0, 64);
	memset(ssid2, 0, 64);
	memset(ch1, 0, 8);
	memset(rssi, 0, 64);
	memset(wan_dhcp, 0, 8);

	/* point pos to extras */
	pos = extras;
	getMyMac(mymac);	// Load our mac address into mymac
	getMyMac2(mymac2);	// Load our mac address into mymac
	
	get_stations(mymac, mymac2, pos);
	pos += strlen(pos);

	get_secure(mymac, mymac2, pos);
	pos += strlen(pos);

	get_users(mymac, mymac2, pos);
	pos += strlen(pos);

	get_uptime(uptime, 64);
	strcat(extras, uptime);
	pos += strlen(pos);

	get_free_mem(freemem, 64);
	strcat(extras, freemem);
	pos += strlen(pos);

	get_load(load, 32);
	strcat(extras, load);
	pos += strlen(pos);

	get_version(version);
	strcat(extras, version);
	pos += strlen(pos);

	get_ssid("wlan0", ssid1, sizeof(ssid1));
	if (strlen(ssid1) > 0) {
		sprintf(pos, "&ssid=%s", ssid1);
		pos += strlen(pos);
	}

	get_ssid("wlan1", ssid2, sizeof(ssid2));
	if (strlen(ssid2) > 0) {
		sprintf(pos, "&pssid=%s", ssid2);
		pos += strlen(pos);
	}

	get_channel("wlan2", ch1, sizeof(ch1));
	if (strlen(ch1) > 0) {
		sprintf(pos, "&ch=%s", ch1);
		pos += strlen(pos);
	}

	getMeshDefaultRouteIp(gw_ip, sizeof(gw_ip) );
	snprintf(pos, sizeof(extras) - strlen(extras), "&gateway=%s", gw_ip);
	pos += strlen(pos);

	get_ip(MESH_IFACE, pos, sizeof(extras) - strlen(extras));
	pos += strlen(pos);
	count_hops(pos, sizeof(extras) - strlen(extras));
	pos += strlen(pos);

	if (isEthernetDefaultRoute()) {
		get_wan(WAN_IFACE, pos, sizeof(extras) - strlen(extras));
			pos += strlen(pos);

		//
		// Ethernet is our default route
		// Add this fact on to our extras
		//
		strcat(extras, "&role=G&gw-qual=96");	// ig = "is gateway"
		system("/sbin/uci set node.general.role=0");

	} else {
		get_rssi(gw_ip, rssi, sizeof(rssi));
		system("/sbin/uci set node.general.role=1");
	}

	if (strlen(rssi) > 0) {
		sprintf(pos, "&grssi=%s", rssi);
		pos += strlen(pos);
	}

	if (set_var_from_shell(wan_dhcp, sizeof(wan_dhcp), "/sbin/uci get dhcp.wan.ignore") != 0) {
		if (strlen(wan_dhcp) > 0) {
			sprintf(pos, "&wan_dhcp=%s", wan_dhcp);
			pos += strlen(pos);
		}
	}

	//
	// Checkin with the central server
	//
	do_checkin(mymac, extras);

	return 0;
}

int check_gw(char* gateway_ip)
{
	char ping_gw[8];
	
	
	if(set_var_from_shell(ping_gw, sizeof(ping_gw), "fping -a 192.5.5.241 192.36.148.17 192.58.128.30  2>/dev/null|wc -l") != 0) {
		return -1;
	}
	
	if(strcmp(ping_gw, "0") == 0) {
		if(strlen(gateway_ip) > 0) {  /* tdimitrov: This should always be true! */
	      		if(strncmp(gateway_ip, "169.254", 7) == 0) {
        			return 1;
	      		} else {
        			/* If I don't have Internet access, but I do have a Gateway
         			 * obtained through dhcp, reboot and wait for service restablishment.
        			 * This helps to avoid a black hole.
         			 */
        			// run_shell_cmd("ifdown old; uci del network.old.ifname; uci set network.new.ifname=eth0; ifup new");
        			return 1;
      			}
    		} else {
			return 1;
    		}
  	} else {
		return 0;
	}
}

int isCableInternetFeed()
{
	char buf[1024];
	char gw_ip[128];
	FILE *fp;
	int isCableInternetFeed;

	debugwrite("Checking if Cable Internet Feed");

	isCableInternetFeed = FALSE;

	if ((fp = popen("netstat -rn", "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		debugwrite("COM Line is ");
		debugwrite(buf);
		debugwrite("\n");
		if (doesStartWith(buf, "0.0.0.0") && doesContain(buf, "eth0")) {
			if(get_wan("eth0", gw_ip, sizeof(gw_ip)) != 0)
				continue;
			
			if(check_gw(gw_ip) == 0) {
				isCableInternetFeed = TRUE;
				debugwrite("YES - Is cable internet feed");
				break;
			}
		}
	}
	pclose(fp);
	
	return isCableInternetFeed;
}

int getMyMac(char *mac)
{
	getMyMacByDev(mac, "wlan0");
	return 0;
}

int getMyMac2(char *mac)
{
	getMyMacByDev(mac, "wlan2");
	return 0;
}

int getMyMacByDev(char *mac, char *dev)
{
	char buf[1024];
	FILE *fp;
	char *tokarray[50];
	int tokloop;
	int wasLastItemHwaddr = FALSE;
	char cmd[32];

	
	// First copy zeros into our mac in case something goes wrong...
	strcpy(mac, "00:00:00:00:00:00");

	sprintf(cmd, "ifconfig %s", dev);
	if ((fp = popen(cmd, "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, fp)) {
		if (doesStartWith(buf, dev)) {
			tokarray[0] = strtok(buf, " ");
			if (NULL == tokarray[0]) {
				// End of tokens
			} else {
				for (tokloop = 1; tokloop < 50; tokloop++) {
					tokarray[tokloop] = strtok(NULL, " ");
					if (NULL == tokarray[tokloop]) {
						//
						// End of tokens
						//
						break;
					}
					if (wasLastItemHwaddr) {
						strcpy(mac, tokarray[tokloop]);
						break;
					}
					if (0 == strcmp(tokarray[tokloop], "HWaddr")) {
						wasLastItemHwaddr = TRUE;
					}
				}
			}
		}
	}
	pclose(fp);

	// Re-format the mac address
	fixmac(mac);

	return 0;
}


int fixmac(char *mytext)
{
	unsigned int i;
	
	
	for (i = 0; i < strlen(mytext); i++) {
                if (mytext[i] == 'a') {
                        mytext[i] = 'A';
		} else if (mytext[i] == 'b') {
			mytext[i] = 'B';
		} else if (mytext[i] == 'c') {
			mytext[i] = 'C';
		} else if (mytext[i] == 'd') {
			mytext[i] = 'D';
		} else if (mytext[i] == 'e') {
			mytext[i] = 'E';
		} else if (mytext[i] == 'f') {
			mytext[i] = 'F';
		}
	}

}

int do_checkin(char *mymac, char *extracgidata)
{
	FILE *fp;
	char cmd[1024];
	char buf[1024];

	char newssid[128] = "err";
	int newchannel = 0;
	char newserver[65] = "err";
	char newurl[65] = "err";
	char first_boot[5];

	http_response *hr = NULL;
	

	// command
	// sprintf(cmd, "/usr/bin/wget -q -O /etc/update/received -t 1 -T 20 \"http://%s/%s?mac=%s%s\"", zServer, zURL, mymac, extracgidata);

	snprintf(cmd, sizeof(cmd), "http://%s/%s?mac=%s%s", zServer, zURL, mymac, extracgidata);
	if ((hr = get_url(cmd, "hdaemon Xi Group/2010")) != NULL) {
		if (hr->is_text && hr->length > 0) {
			if ((fp = fopen("/etc/update/received", "w")) == NULL)
				return -1;

			fwrite(hr->data, hr->length, 1, fp);
			fclose(fp);	
		}
	} else {
		return -1;
	}

	system("/bin/date +%s > /etc/checkin_ok");

	debugwrite("calling check");

	do_check();

	//
	// Has the SSID or Channel been changed?
	//
	if (0 != strcmp(newssid, "err") && 0 != newchannel) {
		if (0 != strcmp(newssid, zSSID) || newchannel != zChannel) {
			//
			// Ok - either the SSID or the Radio channel
			// has been changed!
			//

			// Store the net details in memory
			strcpy(zSSID, newssid);
			zChannel = newchannel;
			strcpy(zServer, newserver);
			strcpy(zURL, newurl);

			// Update our current Radio SSID
		}
	}
	return 0;
}

int get_stations(char *mymac, char *mymac2, char *cgibuf)
{

        char buf[1024];
        int rc;

        FILE *fp;
        char pipe_buf[1024];
        char url_buf[512];
        int ret_val = 0;

        char curr_mac[128];     /* mac address for a cell */
        char curr_signal[8];    /* signal for a cell */
        char rx[8];
        char tx[8];

        int stations_count = 0;

        char* pos_start = NULL;
        char* pos_end = NULL;

        memset(pipe_buf, '\0', sizeof(pipe_buf));

        if ((fp = popen("iwinfo wlan2 assoclist", "r")) == NULL)
                return -1;

        /* we parse something like this:
         * root@OpenWrt:~# iwinfo wlan1 assoclist
         * 00:21:5D:96:F1:34  -51 dBm
         */
        while (fgets(pipe_buf, sizeof(pipe_buf)-1, fp)) {

                /* find the end of the mac field */
                if((pos_end = strchr(pipe_buf, ' ')) == NULL) {
                        ret_val = -2;
                        goto end;
                }

                if(pos_end - pipe_buf + 1 > sizeof(curr_mac)) {
                        ret_val = -3;
                        goto end;
                }

                /* save current mac */
                memset(curr_mac, '\0', sizeof(curr_mac));
                strncpy(curr_mac, pipe_buf, pos_end - pipe_buf);

                /* the end of mac field is the start of rssi field */
                pos_start = pos_end;
                /* skip the spaces after mac */
                while(*pos_start == ' ')
                        pos_start++;

                if((pos_end = strstr(pipe_buf, " dBm")) == NULL) {
                        ret_val = -4;
                        goto end;
                }
                if(pos_end - pos_start + 1 > sizeof(curr_signal)) {
                        ret_val = -5;
                        goto end;
                }

                /* save current signal */
                memset(curr_signal, '\0', sizeof(curr_signal));
                strncpy(curr_signal, pos_start, pos_end - pos_start);

                /* now add data to cgibuf */
                if ((strcmp(mymac, curr_mac) != 0) && (strcmp(mymac2, curr_mac) != 0)) {
                        /* This is one of our stations */
                        stations_count++;

                        memset(rx, '\0', sizeof(rx));
                        memset(tx, '\0', sizeof(tx));
                        if (get_rx_tx_costs(curr_mac, rx, tx, sizeof(rx) - 1) != 0)
                                continue;

                        memset(url_buf, '\0', sizeof(url_buf));
                        sprintf(url_buf, "&n%dm=%s&n%ds=%s&n%drx=%s&n%dtx=%s", stations_count, curr_mac, stations_count, curr_signal, stations_count, rx, stations_count, tx);
                        strcat(cgibuf, url_buf);
                  }
         }

end:
        // Append the total number of stations
        memset(url_buf, '\0', sizeof(url_buf));
        sprintf(url_buf, "&tns=%d", stations_count);
        strcat(cgibuf, url_buf);

        pclose(fp);
        return ret_val;
}

int get_secure(char *mymac, char *mymac2, char *cgibuf)
{
        FILE *fp;
        char pipe_buf[1024];
        char url_buf[512];
        int ret_val = 0;

        char curr_mac[128];     /* mac address for a cell */
        char curr_signal[8];    /* signal for a cell */
        char rx[8];
        char tx[8];

        int stations_count = 0;

        char* pos_start = NULL;
        char* pos_end = NULL;

        memset(pipe_buf, '\0', sizeof(pipe_buf));

        if ((fp = popen("iwinfo wlan1 assoclist", "r")) == NULL)
                return 1;

        /* we parse something like this:
         * root@OpenWrt:~# iwinfo wlan1 assoclist
         * 00:21:5D:96:F1:34  -51 dBm
         */
        while (fgets(pipe_buf, sizeof(pipe_buf)-1, fp)) {
                /* find the end of the mac field */
                if((pos_end = strchr(pipe_buf, ' ')) == NULL) {
                        ret_val = -2;
                        goto end;
                }

                if(pos_end - pipe_buf + 1 > sizeof(curr_mac)) {
                        ret_val = -3;
                        goto end;
                }

                /* save current mac */
                memset(curr_mac, '\0', sizeof(curr_mac));
                strncpy(curr_mac, pipe_buf, pos_end - pipe_buf);

                /* the end of mac field is the start of rssi field */
                pos_start = pos_end;

                /* skip the spaces after mac */
                while(*pos_start == ' ')
                        pos_start++;

                if((pos_end = strstr(pipe_buf, " dBm")) == NULL) {
                        ret_val = -4;
                        goto end;
                }

                if(pos_end - pos_start + 1 > sizeof(curr_signal)) {
                        ret_val = -5;
                        goto end;
                }

                /* save current signal */
                memset(curr_signal, '\0', sizeof(curr_signal));
                strncpy(curr_signal, pos_start, pos_end - pos_start);

                /* now add data to cgibuf */
                if ((strcmp(mymac, curr_mac) != 0) && (strcmp(mymac2, curr_mac) != 0)) {
                        /* This is one of our stations */
                        stations_count++;

                        memset(url_buf, '\0', sizeof(url_buf));
                        sprintf(url_buf, "&1%dm=%s&1%ds=%s", stations_count, curr_mac, stations_count, curr_signal);
                        strcat(cgibuf, url_buf);
                }

        }

end:
        /* Append the total number of users */
        memset(url_buf, '\0', sizeof(url_buf));
        sprintf(url_buf, "&users2=%d", stations_count);
        strcat(cgibuf, url_buf);

        pclose(fp);
        return ret_val;
}

int get_users(char *mymac, char *mymac2, char *cgibuf)
{
        FILE *fp;
        char pipe_buf[1024];
        char url_buf[512];
        int ret_val = 0;

        char curr_mac[128];     /* mac address for a cell */
        char curr_signal[8];    /* signal for a cell */
        char rx[8];
        char tx[8];

        int stations_count = 0;

        char* pos_start = NULL;
        char* pos_end = NULL;

        memset(pipe_buf, '\0', sizeof(pipe_buf));

        if ((fp = popen("iwinfo wlan0 assoclist", "r")) == NULL)
                return 1;

        /* we parse something like this:
         * root@OpenWrt:~# iwinfo wlan1 assoclist
         * 00:21:5D:96:F1:34  -51 dBm
         */
        while (fgets(pipe_buf, sizeof(pipe_buf)-1, fp)) {
                /* find the end of the mac field */
                if((pos_end = strchr(pipe_buf, ' ')) == NULL) {
                        ret_val = -2;
                        goto end;
                }

                if(pos_end - pipe_buf + 1 > sizeof(curr_mac)) {

                        ret_val = -3;
                        goto end;

                /* save current mac */
                memset(curr_mac, '\0', sizeof(curr_mac));
                strncpy(curr_mac, pipe_buf, pos_end - pipe_buf);

                /* the end of mac field is the start of rssi field */
                pos_start = pos_end;

                /* skip the spaces after mac */
                while(*pos_start == ' ')
                        pos_start++;

                if((pos_end = strstr(pipe_buf, " dBm")) == NULL) {
                        ret_val = -4;
                        goto end;
                }

                if(pos_end - pos_start + 1 > sizeof(curr_signal)) {
                        ret_val = -5;
                        goto end;
                }

                /* save current signal */
                memset(curr_signal, '\0', sizeof(curr_signal));
                strncpy(curr_signal, pos_start, pos_end - pos_start);

                /* now add data to cgibuf */
                if ((strcmp(mymac, curr_mac) != 0) && (strcmp(mymac2, curr_mac) != 0)) {
                        /* This is one of our stations */
                        stations_count++;

                        memset(url_buf, '\0', sizeof(url_buf));
                        sprintf(url_buf, "&1%dm=%s&1%ds=%s", stations_count, curr_mac, stations_count, curr_signal);
                        strcat(cgibuf, url_buf);
                }
        }

}

end:
        /* Append the total number of users */
        memset(url_buf, '\0', sizeof(url_buf));
        sprintf(url_buf, "&users1=%d", stations_count);
        strcat(cgibuf, url_buf);

        pclose(fp);
        return ret_val;
}

int doesStartWith(char *bigtext, char *searchtext)
{
	int bigtextlen;
	int searchtextlen;

	
	bigtextlen = strlen(bigtext);
	searchtextlen = strlen(searchtext);

	if (searchtextlen > bigtextlen) {
		return FALSE;
	}

	if (0 == strncmp(bigtext, searchtext, searchtextlen)) {
		return TRUE;
	}
	return FALSE;
}

int doesContain(char *bigtext, char *searchtext)
{
	int res = TRUE;

	
	if (NULL == strstr(bigtext, searchtext)) {
		res = FALSE;
	}

	return res;
}

int doesEndWith(char *bigtext, char *searchtext)
{
	char buf[10];
	int bigtextlen;
	int searchtextlen;

	
	bigtextlen = strlen(bigtext);
	searchtextlen = strlen(searchtext);

	if (searchtextlen > bigtextlen) {
		return FALSE;
	}

	if (bigtextlen < searchtextlen) {
		return FALSE;
	}

	strncpy(buf, bigtext + (bigtextlen - searchtextlen), searchtextlen);

	buf[searchtextlen] = '\0';

	if (0 == strcmp(searchtext, buf)) {
		return TRUE;
	}

	return FALSE;
}


int setAllLedsOn()
{
	debugwrite("setAllLedsOn called");
	return 0;
}

int setAllLedsOff()
{
	debugwrite("setAllLedsOff called");
	return 0;
}

int makeMacUppercase(char *text)
{
	int textlen;
	int i;

	textlen = strlen(text);
	for (i = 0; i < textlen; i++) {
		if (text[i] == 'a') {
			text[i] = 'A';
		}
		if (text[i] == 'b') {
			text[i] = 'B';
		}
		if (text[i] == 'c') {
			text[i] = 'C';
		}
		if (text[i] == 'd') {
			text[i] = 'D';
		}
		if (text[i] == 'e') {
			text[i] = 'E';
		}
		if (text[i] == 'f') {
			text[i] = 'F';
		}
	}
	return 0;
}

int makeMacLowercase(char *text)
{
	int textlen;
	int i;

	textlen = strlen(text);
	for (i = 0; i < textlen; i++) {
		if (text[i] == 'A') {
			text[i] = 'a';
		}
		if (text[i] == 'B') {
			text[i] = 'b';
		}
		if (text[i] == 'C') {
			text[i] = 'c';
		}
		if (text[i] == 'D') {
			text[i] = 'd';
		}
		if (text[i] == 'E') {
			text[i] = 'e';
		}
		if (text[i] == 'F') {
			text[i] = 'f';
		}
	}
	return 0;
}

int debugwrite(char *text)
{
	//
	// Only output debug info if in foreground mode
	//
	if (zIsForeground) {
		printf("hdaemon> %s\n", text);
	}
	return 0;
}

int do_check()
{
	FILE *fp;
	char cmd[1024];
	char buf[1024];

	char newssid[128] = "err";
	int newchannel = 0;
	char newserver[65] = "err";
	char newurl[65] = "err";

	// command
	debugwrite("Doing check config cmd:");
	system("/sbin/check-config");
	debugwrite("done checking config");
	debugwrite("Doing apply_config cmd:");
	system("/usr/sbin/apply_config -a");
	debugwrite(cmd);
	debugwrite("done apply_config");
	return 0;
}

