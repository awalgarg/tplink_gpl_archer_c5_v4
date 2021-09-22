#ifndef __IWPRIV_H__
#define __IWPRIV_H__
int process_iwpriv_cmd(	    
        char *cmd,
        char *dataBuf,
        int dataBufLen);
int init_iwpriv(void);

#endif