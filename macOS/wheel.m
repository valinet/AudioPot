/*
 Wheel - AudioPot "driver" for macOS
 */

// Compile: clang -o wheel wheel.m -framework IOKit -framework Cocoa

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#import <IOKit/hidsystem/ev_keymap.h>

#define BUFFER_SIZE 10

/*
 References:
 https://stackoverflow.com/questions/11045814/emulate-media-key-press-on-mac
 https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
 */

int set_interface_attribs (
    int fd,
    int speed,
    int parity
)
{
        struct termios tty;
        if (tcgetattr (
            fd,
            &tty
        ) != 0)
        {
                printf ("tcgetattr: %d\n", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag |= CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr(
            fd,
            TCSANOW,
            &tty
        ) != 0)
        {
                printf ("tcsetattr: %d\n", errno);
                return -1;
        }
        return 0;
}

void check(int code)
{
	if (!code) exit(0);
}

static io_connect_t get_event_driver(void)
{
	static  mach_port_t sEventDrvrRef = 0;
	mach_port_t masterPort, service, iter;
	kern_return_t    kr;

	if (!sEventDrvrRef)
	{
        	// Get master device port
	        kr = IOMasterPort(
			bootstrap_port,
			&masterPort 
		);
	        check(KERN_SUCCESS == kr);

        	kr = IOServiceGetMatchingServices(
			masterPort, 
			IOServiceMatching(kIOHIDSystemClass),
			&iter
		);
		check(KERN_SUCCESS == kr);

	        service = IOIteratorNext(iter);
	        check(service);

        	kr = IOServiceOpen(
			service,
			mach_task_self(),
			kIOHIDParamConnectType,
			&sEventDrvrRef
		);
	        check(KERN_SUCCESS == kr);

	        IOObjectRelease(service);
	        IOObjectRelease(iter);
	}
	return sEventDrvrRef;
}


static void HIDPostAuxKey(const UInt8 auxKeyCode)
{
	NXEventData event;
	kern_return_t kr;
	IOGPoint loc = { 0, 0 };

	// Key press event
	UInt32 evtInfo = auxKeyCode << 16 | NX_KEYDOWN << 8;
	bzero(
		&event,
		sizeof(NXEventData)
	);
	event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
	event.compound.misc.L[0] = evtInfo;
	kr = IOHIDPostEvent(
		get_event_driver(), 
		NX_SYSDEFINED,
		loc,
		&event,
		kNXEventDataVersion,
		0,
		FALSE
	);
	check(KERN_SUCCESS == kr);

	// Key release event
	evtInfo = auxKeyCode << 16 | NX_KEYUP << 8;
	bzero(
		&event,
		sizeof(NXEventData)
	);
	event.compound.subType = NX_SUBTYPE_AUX_CONTROL_BUTTONS;
	event.compound.misc.L[0] = evtInfo;
	kr = IOHIDPostEvent(
		get_event_driver(),
		NX_SYSDEFINED,
		loc,
		&event,
		kNXEventDataVersion,
		0,
		FALSE
	);
	check(KERN_SUCCESS == kr);
}

int main(
	int argc, 
	char *argv[]
)
{
	int pval = -1, val = -1;
	char *szPortName = "/dev/cu.usbserial-14920";
	char szBuffer[BUFFER_SIZE];
	memset(
		szBuffer, 
		0, 
		BUFFER_SIZE
	);
	char chRd;
	size_t dwPos = 0;

	int fd = open(
		szPortName, 
		O_RDWR | O_NOCTTY | O_SYNC
	);
	if (fd < 0)
	{
		printf("open: %d\n", errno);
		return 0;	
	}
	if (set_interface_attribs(
		fd, 
		B9600,
		0
	) < 0)
	{
		printf("set_interface_attribs\n");
		return 0;
	}
	
	do
	{
		if (read(
			fd, 
			&chRd, 
			1
		) < 0)
		{
			printf("read: %d\n", errno);
			return 0;
		}
		if (chRd == '\r')
		{
            if (!strcmp(szBuffer, "m"))
            {
                //printf("m\n");
                HIDPostAuxKey(NX_KEYTYPE_PLAY);
            }
            else
            {
                pval = val;
                val = atoi(szBuffer) / 61;
                
                //printf("%d\n", val);
                if (pval != val && val == 0)
                {
                    HIDPostAuxKey(NX_KEYTYPE_MUTE);
                }
                else if (val > pval)
                {
                    if (pval == 0)
                    {
                        HIDPostAuxKey(NX_KEYTYPE_MUTE);
                    }
                    else
                    {
                        //printf("+: %d %d\n", val, pval);
                        for (int i = pval; i < val; ++i)
                            HIDPostAuxKey(NX_KEYTYPE_SOUND_UP);
                    }
                }
                else if (val < pval)
                {
                    //printf("-: %d %d\n", val, pval);
                    for (int i = val; i < pval; ++i)
                        HIDPostAuxKey(NX_KEYTYPE_SOUND_DOWN);
                }
            }
			memset(
				szBuffer,
				0,
				BUFFER_SIZE
			);
			dwPos = 0;			
		}
		else if (chRd != '\n')
		{
			szBuffer[dwPos] = chRd;
			if (dwPos < BUFFER_SIZE - 1) dwPos++;
		}
	}
	while (1);		

	close(fd);

	return 0;
}
