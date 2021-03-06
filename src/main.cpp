#include <nfc/nfc.h>

#include <stdlib.h>

static void print_hex(const uint8_t *pbtData, const size_t szBytes)
{
	for(size_t szPos = 0; szPos < szBytes; szPos++)
	{
		printf("%02x  ", pbtData[szPos]);
	}
	printf("\n");
}

int main(int argc, const char *argv[])
{
	nfc_device *pnd;
	nfc_target nt;
	nfc_context *context;

	nfc_init(&context);
	if (context == NULL)
	{
		printf("Unable to init libnfc\n");
		exit(EXIT_FAILURE);
	}

	const char *acLibnfcVersion = nfc_version();
	(void)argc;
	printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);

	// Open, using the first available NFC device which can be in order of selection:
	// - default device specified using environment variable or
	// - first specified device in libnfc.conf (/etc/nfc) or
	// - first specified device in device-configuration directory (/etc/nfc/devices.d) or
	// - first auto-detected (if feature is not disabled in libnfc.conf) device
	pnd = nfc_open(context, NULL);

	if(pnd == NULL)
	{
		printf("ERROR: Unable to open NFC device\n");
		exit(EXIT_FAILURE);
	}

	if(nfc_initiator_init(pnd) < 0)
	{
		nfc_perror(pnd, "nfc_initiator_init");
		exit(EXIT_FAILURE);
	}

	printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

	const nfc_modulation nmMifare =
	{
		.nmt = NMT_FELICA,
		.nbr = NBR_106,
	};

	if(nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) > 0)
	{
		printf("The following tag was found:\n");
		printf("    ATQA (SENS_RES): ");
		print_hex(nt.nti.nai.abtAtqa, 2);
		printf("       UID (NFCID%c): ", (nt.nti.nai.abtUid[0] == 0x08 ? '3' : '1'));
		print_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
		printf("      SAK (SEL_RES): ");
		print_hex(&nt.nti.nai.btSak, 1);
		if(nt.nti.nai.szAtsLen)
		{
			printf("          ATS (ATR): ");
			print_hex(nt.nti.nai.abtAts, nt.nti.nai.szAtsLen);
		}
	}

	nfc_close(pnd);
	nfc_exit(context);
	exit(EXIT_SUCCESS);
}





