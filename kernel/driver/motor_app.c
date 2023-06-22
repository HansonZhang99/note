#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>

typedef enum 
{
	MOTOR_CONTROL_FORWARD = (1 << 10) + 1,
	MOTOR_CONTROL_RESERVE,
	MOTOR_CONTROL_ON,
	MOTOR_CONTROL_OFF,
	MOTOR_CONTROL_SPEED,
}motor_control_cmd;

int main()
{
	int fd;
	unsigned long flag;
	int rv;
	int i = 2;
	fd = open("/dev/motor_control", O_RDWR);

	printf("open /dev/motor_control\n");


	rv = ioctl(fd, MOTOR_CONTROL_ON, &flag);
	printf("motor control on, rv= %d\n", rv);
	sleep(5);
	
	flag = 15;
	ioctl(fd, MOTOR_CONTROL_SPEED, &flag);
	printf("motor control speed = %d, rv = %d\n", flag, rv);
	sleep(5);
	flag = 2;
	ioctl(fd, MOTOR_CONTROL_SPEED, &flag);
	printf("motor control speed = %d, rv = %d\n", flag, rv);
	while(i--)
	{
		sleep(5);
		rv = ioctl(fd, MOTOR_CONTROL_RESERVE, &flag);
		printf("motor control reserve, rv= %d\n", rv);
		sleep(5);
		rv = ioctl(fd, MOTOR_CONTROL_FORWARD, &flag);
		printf("motor control forward, rv= %d\n", rv);
	}
	rv = ioctl(fd, MOTOR_CONTROL_OFF, &flag);
	printf("motor control off, rv= %d\n", rv);
	close(fd);
	return 0;
}
