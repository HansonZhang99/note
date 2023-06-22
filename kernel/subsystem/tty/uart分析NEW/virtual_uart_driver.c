#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/rational.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>

#include <asm/irq.h>

static struct uart_port *virt_port;
static const struct of_device_id virtual_uart_table[] = {
    { .compatible = "virtual_uart" },
    { },
};
#define BUF_LEN  1024
#define NEXT_PLACE(i) ((i+1)&0x3FF)

static unsigned char tx_buf[BUF_LEN];
static unsigned int tx_read_idx = 0;
static unsigned int tx_write_idx = 0;

static unsigned char rx_buf[BUF_LEN];
//static unsigned int rx_read_idx = 0;
static unsigned int rx_write_idx = 0;

struct proc_dir_entry *virt_uart_proc;
static int is_txbuf_empty(void)
{
	return tx_read_idx == tx_write_idx;
}

static int is_txbuf_full(void)
{
	return NEXT_PLACE(tx_write_idx) == tx_read_idx;
}

static int txbuf_put(unsigned char val)
{
	if (is_txbuf_full())
		return -1;
	tx_buf[tx_write_idx] = val;
	tx_write_idx = NEXT_PLACE(tx_write_idx);
	return 0;
}

static int txbuf_get(unsigned char *pval)
{
	if (is_txbuf_empty())
		return -1;
	*pval = tx_buf[tx_read_idx];
	tx_read_idx = NEXT_PLACE(tx_read_idx);
	return 0;
}

static int txbuf_count(void)
{
	if (tx_write_idx >= tx_read_idx)
		return tx_write_idx - tx_read_idx;
	else
		return BUF_LEN + tx_write_idx - tx_read_idx;
}


static struct uart_driver virtual_uart_reg = {
	.owner          = THIS_MODULE,
	.driver_name    = "ttyVirDriver",
	.dev_name       = "ttyVirtual",
	.major          = 0,
	.minor          = 0,
	.nr             = 1,
};
static unsigned int virtual_uart_tx_empty(struct uart_port *port)
{
	return 1;
}	


static void virtual_uart_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

	while (!uart_circ_empty(xmit) && !uart_tx_stopped(port)){
		/* send xmit->buf[xmit->tail]
 		* out the port here */
		txbuf_put(xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

}


static void virtual_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		   struct ktermios *old)
{
	return ;
}

static int virtual_uart_startup(struct uart_port *port)
{
	return 0;
}
static void virtual_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	return ;
}

static unsigned int virtual_uart_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void virtual_uart_stop_tx(struct uart_port *port)
{
	return ;
}
static void virtual_uart_stop_rx(struct uart_port *port)
{
	return ;
}

static void virtual_uart_shutdown(struct uart_port *port)
{
	return ;
}

static const char *virtual_uart_type(struct uart_port *port)
{
	return "VIRT_UART_TYPE";
}

static const struct uart_ops virtual_uart_pops = {
	.tx_empty	= virtual_uart_tx_empty,
	.set_mctrl	= virtual_uart_set_mctrl,
	.get_mctrl	= virtual_uart_get_mctrl,
	.stop_tx	= virtual_uart_stop_tx,
	.start_tx	= virtual_uart_start_tx,
	.stop_rx	= virtual_uart_stop_rx,
	//.enable_ms	= imx_enable_ms,
	//.break_ctl	= imx_break_ctl,
	.startup	= virtual_uart_startup,
	.shutdown	= virtual_uart_shutdown,
	//.flush_buffer	= imx_flush_buffer,
	.set_termios	= virtual_uart_set_termios,
	.type		= virtual_uart_type,
	//.config_port	= imx_config_port,
	//.verify_port	= imx_verify_port,

};

static ssize_t virtual_uart_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	int cnt;
	unsigned char val;
	int i;
	int ret;
	cnt = txbuf_count();
	cnt = cnt > size ? size : cnt;
	for(i = 0; i < cnt; i++)
	{
		txbuf_get(&val);
		ret = copy_to_user(buf + i, &val, 1);
	}
	return cnt;
}

static ssize_t virtual_uart_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	int ret;
	ret = copy_from_user(rx_buf, buf, size);
	rx_write_idx = size;

	irq_set_irqchip_state(virt_port->irq, IRQCHIP_STATE_PENDING, 1);
	return size;
}


static const struct file_operations virtual_uart_fops = {
	//.open		= cmdline_proc_open,
	.read		= virtual_uart_read,
	.write		= virtual_uart_write,
	//.llseek		= seq_lseek,
	//.release	= single_release,
};

static irqreturn_t virt_uart_rxint(int irq, void *dev_id)
{
	struct tty_port *port = &virt_port->state->port;
	unsigned long flags;
	unsigned int i = 0;

	spin_lock_irqsave(&virt_port->lock, flags);

	while (i < rx_write_idx) {
		virt_port->icount.rx++;
		if (tty_insert_flip_char(port, rx_buf[i++], TTY_NORMAL) == 0)
			virt_port->icount.buf_overrun++;
	}
	rx_write_idx = 0;

	spin_unlock_irqrestore(&virt_port->lock, flags);
	tty_flip_buffer_push(port);
	return IRQ_HANDLED;
}


static int virtual_uart_probe(struct platform_device *pdev)
{
	int rxirq;
	int ret;
	
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	virt_uart_proc = proc_create("virtual_uart", 0, NULL, &virtual_uart_fops);
	rxirq = platform_get_irq(pdev, 0);

	virt_port = devm_kzalloc(&pdev->dev, sizeof(*virt_port), GFP_KERNEL);

	virt_port->dev = &pdev->dev;

	virt_port->type = PORT_IMX,
	virt_port->iotype = UPIO_MEM;
	virt_port->irq = rxirq;
	virt_port->fifosize = 32;
	virt_port->type = PORT_8250;
	virt_port->ops = &virtual_uart_pops;
	//virt_port->rs485_config = imx_rs485_config;
	//virt_port->rs485.flags =
	//	SER_RS485_RTS_ON_SEND | SER_RS485_RX_DURING_TX;
	virt_port->flags = UPF_BOOT_AUTOCONF;

	ret = devm_request_irq(&pdev->dev, rxirq, virt_uart_rxint, 0,
				       dev_name(&pdev->dev), virt_port);
	return uart_add_one_port(&virtual_uart_reg, virt_port);
}

static int virtual_uart_remove(struct platform_device *pdev)
{
	uart_remove_one_port(&virtual_uart_reg, virt_port);
	proc_remove(virt_uart_proc);
	return 0;
}

static struct platform_driver virtual_uart_driver =
{
	.probe      = virtual_uart_probe,
    .remove     = virtual_uart_remove,
    .driver     = {
        .name   = "virtual_uart",
        .of_match_table = virtual_uart_table,
    },
};



static int __init virtual_uart_init(void)
{
	int ret = -1;
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	
	ret = uart_register_driver(&virtual_uart_reg);
	if(ret)
	{
		printk("uart_register_driver error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	ret = platform_driver_register(&virtual_uart_driver);
	if(ret)
	{
		printk("platform_driver_register error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	return 0;
}

static void __exit virtual_uart_exit(void)
{
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	platform_driver_unregister(&virtual_uart_driver);
	uart_unregister_driver(&virtual_uart_reg);
	
}

module_init(virtual_uart_init);
module_exit(virtual_uart_exit);

MODULE_LICENSE("GPL");

