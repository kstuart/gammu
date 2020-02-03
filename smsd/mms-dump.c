#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "streambuffer.h"
#include "mms-data.h"
#include "mms-service.h"

void DumpEncoded(SBUFFER stream);
void DumpParts(SBUFFER stream, MMSPARTS parts);

int main(int argc, char **argv)
{
	int status = 0;
	puts("Dump MMS v1.0\n");

	if(argc != 2) {
		puts("Usage:\n  dump-mms <mms dump file>\n  Prints information about provided MMS dump file.\n");
		exit(0);
	}

	const char *file = argv[1];

	struct stat st;
	SBUFFER mms = NULL;

	if(stat(file, &st) != 0) {
		printf("Aborting: Failed to stat file %s\n  - %s\n", file, strerror(errno));
		exit(-1);
	}

	mms = SB_InitWithCapacity(st.st_size / 2);
	if(!mms) {
		printf("Aborting: Failed to allocate buffer for MMS\n  - %s\n", strerror(errno));
		exit(-1);
	}

	int fd = open(file, O_RDONLY);
	if(fd == -1) {
		printf("Aborting: Failed to open %s\n  - %s\n", file, strerror(errno));
		exit(-1);
	}

	if(SB_Read(mms, fd, st.st_size) == -1) {
		printf("Aborting: Failed to read %s\n  - %s\n", file, strerror(errno));
		exit(-1);
	}
	close(fd);

	DumpEncoded(mms);
	SB_Destroy(&mms);

	return status;
}

void DumpParts(SBUFFER stream, MMSPARTS parts)
{
	assert(stream);
	assert(parts);

	MMSPART p;

	for(size_t i = 0; i < parts->end; i++) {
		p = &parts->entries[i];
		SB_PutFormattedString(stream,"Part: %zu - Data size: %zu bytes\n", i + 1, p->data_len);
		MMS_DumpHeaders(stream, p->headers);
	}
}

void DumpEncoded(SBUFFER stream)
{
	assert(stream);
	SBUFFER out = SB_InitWithCapacity(20240);

	MMSMESSAGE m = NULL;
	MMS_MapEncodedMessage(NULL, stream, &m);
	MMS_DumpHeaders(out, m->Headers);
	SB_PutByte(stream, '\n');
	DumpParts(out, m->Parts);

	SB_PutByte(out, 0);
	puts(SBBase(out));

	MMSMessage_Destroy(&m);
	SB_Destroy(&out);
}
