

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define NO_OF_GROUPS 2
#define NO_OF_STATES 2
#define NO_OF_READERS 1

#define SIG_P2C 1
#define SPI_PATH "/dev/spidev0.1"
int spi_id;
int flag = 0;

uint16_t out_sensor[3];
uint16_t out_kalman[3];
int duty_cycle;

int fd;
// define placeholder functions
void kalmanfilterR(uint16_t *buf, uint16_t *buf2) { return; }
void kalmanfilterS(uint16_t *buf, uint16_t *buf2) { return; }
int pid(uint16_t *buf2) { return 0; }
int pwm(int duty_cycle) { return 0; }

// make struct for each state
typedef struct
{
        pid_t pid;
        struct row *head;
        int length;
} state;
// make struct for each row for each state
struct row
{
        uint8_t bitmask1[NO_OF_GROUPS];
        uint8_t bitmask0[NO_OF_GROUPS];
        state *next_state;
};
typedef struct row row;
uint8_t input_mask[2] = {0x00, 0x00};
uint8_t clear_mask[2] = {0xFF, 0xF0};
row stat_rows[2];
row rot_rows[2];
state stationary_state = {0, stat_rows, 1};
state rotate_state = {0, rot_rows, 1};
state *current_state;
state *next_state;

// add a row to the state table for a particular state
int add_row(row *head, uint8_t bitmask1, uint8_t bitmask0, int group_no, int row_no,
            state *next_state)
{
        // static int index;
        head[row_no].bitmask1[group_no] = bitmask1;
        head[row_no].bitmask0[group_no] = bitmask0;
        head[row_no].next_state = next_state;
        return 0;
}
// create the state table
int init_modes()
{
        add_row(stat_rows, 0xFF, 0x01, 0, 0, &rotate_state);
        add_row(stat_rows, 0xFF, 0x00, 1, 0, &rotate_state);
        // maybe add row that leads to the current state itself?
        add_row(rot_rows, 0xFF, 0x01, 0, 0, &stationary_state);
        add_row(rot_rows, 0xFF, 0x00, 1, 0, &stationary_state);
        return 0;
}

int compare_input_mask(uint8_t inputmask[], row row)
{
        for (int i = 0; i < NO_OF_GROUPS; i++)
        {
                if (~((inputmask[i] & row.bitmask1[i]) | (~inputmask[i] & ~row.bitmask0[i])))
                {
                        return 0;
                }
        }
        return 1;
}

// if A: input
//    B: bitmask1
//    C: bitmask0
//    then (AB + A'C')' = 0 will give the next state to go to
void sigstophandler(int signo, siginfo_t *info, void *context)
{
        if (signo == SIGTSTP)
        {
                flag = 1;
        }
}

void spi_sigusrhandler(int signo, siginfo_t *info, void *context)
{
        union sigval sv;
        sv.sival_int = 0x0100F0F1;
        if (sigqueue(getppid(), SIGUSR1, sv) < 0)
        {
                perror("sigqueue");
                return;
        }
}

void sigusrhandler(int signo, siginfo_t *info, void *context)
{
        uint32_t sigv = info->si_value.sival_int;
        uint8_t address1 = (sigv & 0xFF000000) >> 24;
        uint8_t address0 = (sigv & 0x00FF0000) >> 16;
        uint8_t bitmask1_1 = (address1 % 2) ? ((sigv & 0x0000F000) >> 8) | 0x0F
                                            : ((sigv & 0x0000F000) >> 12) | 0xF0;
        uint8_t bitmask0_1 = (sigv & 0x00000F00) >> 8; // bitmask0 of data1
        uint8_t bitmask1_0 = (address0 % 2) ? (sigv & 0x000000F0) | 0x0F
                                            : ((sigv & 0x000000F0) >> 4) | 0xF0;
        uint8_t bitmask0_0 = sigv & 0x0000000F;

        printf("[PARENT] SIGUSR1 received (sigv=0x%x)\n", sigv);
        printf("         address1=%d, address0=%d\n", address1, address0);
        printf("         bitmask1_1=0x%x bitmask0_1=0x%x\n", bitmask1_1, bitmask0_1);
        printf("         bitmask1_0=0x%x bitmask0_0=0x%x\n", bitmask1_0, bitmask0_0);

        input_mask[address1 / 2] = (input_mask[address1 / 2] & bitmask1_1) |
                                   (input_mask[address1 / 2] & bitmask0_1) |
                                   (bitmask1_1 & bitmask0_1);
        input_mask[address0 / 2] = (input_mask[address0 / 2] & bitmask1_0) |
                                   (input_mask[address0 / 2] & bitmask0_0) |
                                   (bitmask1_0 & bitmask0_0);
        for (int i = 0; i < current_state->length; i++)
        {
                if (compare_input_mask(input_mask, current_state->head[i]))
                {
                        next_state = current_state->head[i].next_state;
                        break;
                }
        }
        if (next_state == current_state)
        {
                printf("[PARENT] No state change\n");
                return;
        }

        printf("[PARENT] Transition %s → %s\n",
               (current_state == &stationary_state ? "STATIONARY" : "ROTATE"),
               (next_state == &stationary_state ? "STATIONARY" : "ROTATE"));

        kill(current_state->pid, SIGTSTP);
        int status;
        if (waitpid(current_state->pid, &status, WUNTRACED) < 0)
        {
                perror("waitpid");
                exit(1);
        }
        if (WIFSTOPPED(status))
        {
                kill(next_state->pid, SIGCONT);
                if (waitpid(next_state->pid, &status, WCONTINUED) < 0)
                {
                        perror("waitpid");
                        exit(1);
                }
                if (WIFCONTINUED(status))
                {
                        current_state = next_state;
                }
        }
        for (int i = 0; i < NO_OF_GROUPS; i++)
        {
                input_mask[i] = input_mask[i] & clear_mask[i];
        }
}

int main()
{
        /* Dummy SPI process */
        if (!(spi_id = fork()))
        {
                printf("[SPI] Dummy SPI process running (PID=%d)\n", getpid());
                printf("[SPI] Send SIGUSR1 here. I will forward a test signal to the parent (%d)\n", getppid());

                struct sigaction act;
                act.sa_sigaction = spi_sigusrhandler;
                act.sa_flags = SA_SIGINFO;
                sigemptyset(&act.sa_mask);

                if (sigaction(SIGUSR1, &act, NULL) < 0)
                {
                        perror("spi sigaction");
                        exit(1);
                }
                while (1)
                        pause();
        }
        else if (spi_id < 0)
                goto error;

        /* ROTATE state */
        else if (!(rotate_state.pid = fork()))
        {
                struct sigaction act;
                act.sa_sigaction = sigstophandler;
                act.sa_flags = SA_SIGINFO;
                sigemptyset(&act.sa_mask);
                if (sigaction(SIGTSTP, &act, NULL) < 0)
                {
                        perror("sigaction");
                        exit(1);
                }
                printf("[ROTATE] Child PID=%d started\n", getpid());
                raise(SIGSTOP);
                while (1)
                {
                        if (flag)
                        {
                                flag = 0;
                                raise(SIGSTOP);
                        }
                        printf("[ROTATE] Running...\n");
                        sleep(2);
                }
        }
        else if (rotate_state.pid < 0)
                goto error;

        /* STATIONARY state */
        else if (!(stationary_state.pid = fork()))
        {
                struct sigaction act;
                act.sa_sigaction = sigstophandler;
                act.sa_flags = SA_SIGINFO;
                sigemptyset(&act.sa_mask);
                if (sigaction(SIGTSTP, &act, NULL) < 0)
                {
                        perror("sigaction");
                        exit(1);
                }
                printf("[STATIONARY] Child PID=%d started\n", getpid());
                raise(SIGSTOP);
                while (1)
                {
                        if (flag)
                        {
                                flag = 0;
                                raise(SIGSTOP);
                        }
                        printf("[STATIONARY] Running...\n");
                        sleep(2);
                }
        }
        else if (stationary_state.pid < 0)
                goto error;

        /* PARENT */
        else
        {
                init_modes();
                int status;
                struct sigaction act;
                act.sa_sigaction = sigusrhandler;
                act.sa_flags = SA_SIGINFO;
                if (sigaction(SIGUSR1, &act, NULL) < 0)
                {
                        perror("sigaction");
                        exit(1);
                }

                // wait for both children to stop
                for (int i = 0; i < NO_OF_STATES; i++)
                {
                        if (waitpid(-1, &status, WUNTRACED) < 0)
                        {
                                perror("waitpid");
                                exit(1);
                        }
                }

                // start in ROTATE
                kill(rotate_state.pid, SIGCONT);
                if (waitpid(rotate_state.pid, &status, WCONTINUED) < 0)
                {
                        perror("waitpid");
                        exit(1);
                }
                if (WIFCONTINUED(status))
                {
                        current_state = next_state = &rotate_state;
                        printf("[PARENT] Initial state: ROTATE\n");
                }

                while (1)
                        pause();
        }

error:
        perror("fork");
        exit(1);
}

