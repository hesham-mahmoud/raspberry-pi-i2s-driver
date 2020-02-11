/* File: i2s_driver.h
 * Author: TR
 * Description: Header file for i2s_driver
 * Version History
 * v1.0 Initial release
*/

/* Register macros, function prototypes, and structures for the I2S interface */

#ifndef I2S_DRIVER_H
#define I2S_DRIVER_H

#include <linux/ioctl.h>    // For ioctl macros

/*************************** BEGIN IOCTL DECLARATIONS **************************/

#define I2S_SET_EN          _IOW('i', 0, char)
#define I2S_SET_TXON        _IOW('i', 1, char)
#define I2S_SET_RXON        _IOW('i', 2, char)
#define I2S_TX_BUFF_SPACE   _IOR('i', 3, int)
#define I2S_RX_BUFF_ITEMS   _IOR('i', 4, int)
#define I2S_CLEAR_TX_BUFF   _IOW('i', 5, char)
#define I2S_CLEAR_RX_BUFF   _IOW('i', 6, char)
#define I2S_WRITE_CS_A      _IOW('i', 7, uint32_t)
#define I2S_WRITE_MODE_A    _IOW('i', 8, uint32_t)
#define I2S_WRITE_RXC_A     _IOW('i', 9, uint32_t)
#define I2S_WRITE_TXC_A     _IOW('i', 10, uint32_t)
#define I2S_WRITE_DREQ_A    _IOW('i', 11, uint32_t)
#define I2S_WRITE_INTEN_A   _IOW('i', 12, uint32_t)
#define I2S_WRITE_INTSTC_A  _IOW('i', 13, uint32_t)
#define I2S_WRITE_GRAY      _IOW('i', 14, uint32_t)
#define I2S_CLR_TX_FIFO     _IOW('i', 15, char)
#define I2S_CLR_RX_FIFO     _IOW('i', 16, char)

/*************************** END IOCTL DECLARATIONS **************************/

#define DEVICE_NAME "i2s_driver"

/* Switch these depending on which version of the RaspPi you're using */
//#define RPIZERO   // For original Raspberry Pi and Zero
#define RPITWO    // For Raspberry Pi 2 or 3

/* Address Definitions */
#ifdef RPIZERO
  #define PI_PERIPHERAL_BASE        0x20000000
#elif defined(RPITWO)
  #define PI_PERIPHERAL_BASE        0x3F000000	// 7E seems not right
#endif

#define I2S_OFFSET                0x00203000
#define I2S_BASE                  PI_PERIPHERAL_BASE + I2S_OFFSET
#define I2S_SIZE                  0x24          // Number of bytes used by the I2S registers

/************************* BEGIN STRUCT DECLARATIONS **************************/

struct i2s_inst {
  uint32_t CS_A;        // Control and status
  uint32_t FIFO_A;      // FIFO data
  uint32_t MODE_A;      // Mode control
  uint32_t RXC_A;       // Receive config
  uint32_t TXC_A;       // Transmit config
  uint32_t DREQ_A;      // DMA request level
  uint32_t INTEN_A;     // Interrupt enables
  uint32_t INTSTC_A;    // Interrupt status and clear
  uint32_t GRAY;        // Gray mode control
};

/* FIFO for holding I2S data in the kernel space */
struct i2s_buffer {
  int32_t *buffer;
  unsigned int head;
  unsigned int tail;
  unsigned int size;
};

/************************END STRUCT DECLARATIONS ******************************/

/*********************** BEGIN I2S DECLARATIONS ******************************/

/* Commented section not needed due to the use of a structure */

/* PCM Address Map Offsets */
// #define I2S_CS_A_OFFSET           0x0
// #define I2S_FIFO_A_OFFSET         0x4
// #define I2S_MODE_A_OFFSET         0x8
// #define I2S_RXC_A_OFFSET          0xC
// #define I2S_TXC_A_OFFSET          0x10
// #define I2S_DREQ_A_OFFSET         0x14
// #define I2S_INTEN_A_OFFSET        0x18
// #define I2S_INTSTC_A_OFFSET       0x1C
// #define I2S_GRAY_OFFSET           0x20

/* PCM Register Addresses */
// #define I2S_CS_A                  (I2S_BASE + I2S_CS_A_OFFSET)
// #define I2S_FIFO_A                (I2S_BASE + I2S_FIFO_A_OFFSET)
// #define I2S_MODE_A                (I2S_BASE + I2S_MODE_A_OFFSET)
// #define I2S_RXC_A                 (I2S_BASE + I2S_RXC_A_OFFSET)
// #define I2S_TXC_A                 (I2S_BASE + I2S_TXC_A_OFFSET)
// #define I2S_DREQ_A                (I2S_BASE + I2S_DREQ_A_OFFSET)
// #define I2S_INTEN_A               (I2S_BASE + I2S_INTEN_A_OFFSET)
// #define I2S_INTSTC_A              (I2S_BASE + I2S_INTSTC_A_OFFSET)
// #define I2S_GRAY                  (I2S_BASE + I2S_GRAY_OFFSET)

/* Bits for CS_A Register */
#define I2S_CS_A_STBY             (0x1u << 25)
#define I2S_CS_A_SYNC             (0x1u << 24)
#define I2S_CS_A_RXSEX            (0x1u << 23)
#define I2S_CS_A_RXF_MASK         (0x1u << 22)
#define I2S_CS_A_TXE_MASK         (0x1u << 21)
#define I2S_CS_A_RXD_MASK         (0x1u << 20)
#define I2S_CS_A_TXD_MASK         (0x1u << 19)
#define I2S_CS_A_RXR_MASK         (0x1u << 18)
#define I2S_CS_A_TXW_MASK         (0x1u << 17)
#define I2S_CS_A_RXERR            (0x1u << 16)
#define I2S_CS_A_TXERR            (0x1u << 15)
#define I2S_CS_A_RXSYNC_MASK      (0x1u << 14)
#define I2S_CS_A_TXSYNC_MASK      (0x1u << 13)
#define I2S_CS_A_DMAEN            (0x1u << 9)
#define I2S_CS_A_RXTHR(val)       ((val << 7) & (0x3 << 7))
#define I2S_CS_A_TXTHR(val)       ((val << 5) & (0x3 << 5))
#define I2S_CS_A_RXCLR            (0x1u << 4)
#define I2S_CS_A_TXCLR            (0x1u << 3)
#define I2S_CS_A_TXON             (0x1u << 2)
#define I2S_CS_A_RXON             (0x1u << 1)
#define I2S_CS_A_EN               (0x1u << 0)

/* Bits for MODE_A Register */
#define I2S_MODE_A_CLK_DIS        (0x1u << 28)
#define I2S_MODE_A_PDMN           (0x1u << 27)
#define I2S_MODE_A_PDME           (0x1u << 26)
#define I2S_MODE_A_FRXP           (0x1u << 25)
#define I2S_MODE_A_FTXP           (0x1u << 24)
#define I2S_MODE_A_CLKM           (0x1u << 23)
#define I2S_MODE_A_CLKI           (0x1u << 22)
#define I2S_MODE_A_FSM            (0x1u << 21)
#define I2S_MODE_A_FSI            (0x1u << 20)
#define I2S_MODE_A_FLEN_POS       (10)
#define I2S_MODE_A_FLEN_MASK      (0x3FF << I2S_MODE_A_FLEN_POS)
#define I2S_MODE_A_FLEN(val)      (I2S_MODE_A_FLEN_MASK & (val << I2S_MODE_A_FLEN_POS))
#define I2S_MODE_A_FSLEN_POS      (0)
#define I2S_MODE_A_FSLEN_MASK     (0x3FF << I2S_MODE_A_FSLEN_POS)
#define I2S_MODE_A_FSLEN(val)     (I2S_MODE_A_FSLEN_MASK & (val << I2S_MODE_A_FSLEN_POS))

/* Bits for RXC_A Register */
#define I2S_RXC_A_CH1WEX          (0x1u << 31)
#define I2S_RXC_A_CH1EN           (0x1u << 30)
#define I2S_RXC_A_CH1POS_POS      (20)
#define I2S_RXC_A_CH1POS_MASK     (0x3FF << I2S_RXC_A_CH1POS_POS)
#define I2S_RXC_A_CH1POS(val)     (I2S_RXC_A_CH1POS_MASK & (val << I2S_RXC_A_CH1POS_POS))
#define I2S_RXC_A_CH1WID_POS      (16)
#define I2S_RXC_A_CH1WID_MASK     (0xFu << I2S_RXC_A_CH1WID_POS)
#define I2S_RXC_A_CH1WID(val)     (I2S_RXC_A_CH1WID_MASK & (val << I2S_RXC_A_CH1WID_POS))
#define I2S_RXC_A_CH2WEX          (0x1u << 15)
#define I2S_RXC_A_CH2EN           (0x1u << 14)
#define I2S_RXC_A_CH2POS_POS      (4)
#define I2S_RXC_A_CH2POS_MASK     (0x3FFu << I2S_RXC_A_CH2POS_POS)
#define I2S_RXC_A_CH2POS(val)     (I2S_RXC_A_CH2POS_MASK & (val << I2S_RXC_A_CH2POS_POS))
#define I2S_RXC_A_CH2WID_POS      (0)
#define I2S_RXC_A_CH2WID_MASK     (0xFu << I2S_RXC_A_CH2WID_POS)
#define I2S_RXC_A_CH2WID(val)     (I2S_RXC_A_CH2WID_MASK & (val << I2S_RXC_A_CH2WID_POS))

/* Bits for TXC_A Register */
#define I2S_TXC_A_CH1WEX          (0x1u << 31)
#define I2S_TXC_A_CH1EN           (0x1u << 30)
#define I2S_TXC_A_CH1POS_POS      (20)
#define I2S_TXC_A_CH1POS_MASK     (0x3FF << I2S_TXC_A_CH1POS_POS)
#define I2S_TXC_A_CH1POS(val)     (I2S_TXC_A_CH1POS_MASK & (val << I2S_TXC_A_CH1POS_POS))
#define I2S_TXC_A_CH1WID_POS      (16)
#define I2S_TXC_A_CH1WID_MASK     (0xFu << I2S_TXC_A_CH1WID_POS)
#define I2S_TXC_A_CH1WID(val)     (I2S_TXC_A_CH1WID_MASK & (val << I2S_TXC_A_CH1WID_POS))
#define I2S_TXC_A_CH2WEX          (0x1u << 15)
#define I2S_TXC_A_CH2EN           (0x1u << 14)
#define I2S_TXC_A_CH2POS_POS      (4)
#define I2S_TXC_A_CH2POS_MASK     (0x3FFu << I2S_TXC_A_CH2POS_POS)
#define I2S_TXC_A_CH2POS(val)     (I2S_TXC_A_CH2POS_MASK & (val << I2S_TXC_A_CH2POS_POS))
#define I2S_TXC_A_CH2WID_POS      (0)
#define I2S_TXC_A_CH2WID_MASK     (0xFu << I2S_TXC_A_CH2WID_POS)
#define I2S_TXC_A_CH2WID(val)     (I2S_TXC_A_CH2WID_MASK & (val << I2S_TXC_A_CH2WID_POS))

/* Bits for DREQ_A Register */
#define I2S_DREQ_A_TX_PANIC_POS   (24)
#define I2S_DREQ_A_TX_PANIC_MASK  (0x7Fu << I2S_DREQ_A_TX_PANIC_POS)
#define I2S_DREQ_A_TX_PANIC(val)  (I2S_DREQ_A_TX_PANIC_MASK & (val << I2S_DREQ_A_TX_PANIC_POS))
#define I2S_DREQ_A_RX_PANIC_POS   (16)
#define I2S_DREQ_A_RX_PANIC_MASK  (0x7Fu << I2S_DREQ_A_RX_PANIC_POS)
#define I2S_DREQ_A_RX_PANIC(val)  (I2S_DREQ_A_RX_PANIC_MASK & (val << I2S_DREQ_A_RX_PANIC_POS))
#define I2S_DREQ_A_TX_POS         (8)
#define I2S_DREQ_A_TX_MASK        (0x7Fu << I2S_DREQ_A_TX_POS)
#define I2S_DREQ_A_TX(val)        (I2S_DREQ_A_TX_MASK & (val << I2S_DREQ_A_TX_POS))
#define I2S_DREQ_A_RX_POS         (0)
#define I2S_DREQ_A_RX_MASK        (0x7Fu << I2S_DREQ_A_RX_POS)
#define I2S_DREQ_A_RX(val)        (I2S_DREQ_A_RX_MASK & (val << I2S_DREQ_A_RX_POS))

/* Bits for INTEN_A Register */
#define I2S_INTEN_A_RXERR         (0x1u << 3)
#define I2S_INTEN_A_TXERR         (0x1u << 2)
#define I2S_INTEN_A_RXR           (0x1u << 1)
#define I2S_INTEN_A_TXW           (0x1u << 0)

/* Bits for INTSTC_A Register */
#define I2S_INTSTC_A_RXERR        (0x1u << 3)
#define I2S_INTSTC_A_TXERR        (0x1u << 2)
#define I2S_INTSTC_A_RXR          (0x1u << 1)
#define I2S_INTSTC_A_TXW          (0x1u << 0)

/* Bits for GRAY Register */
#define I2S_GRAY_RXFIFOLEVEL_POS  (16)
#define I2S_GRAY_RXFIFOLEVEL_MASK (0x3Fu << I2S_GRAY_RXFIFOLEVEL_POS)
#define I2S_GRAY_FLUSHED_POS      (10)
#define I2S_GRAY_FLUSHED_MASK     (0x3Fu << I2S_GRAY_FLUSHED_POS)
#define I2S_GRAY_RXLEVEL_POS      (4)
#define I2S_GRAY_RXLEVEL_MASK     (0x3Fu << I2S_GRAY_RXLEVEL_POS)
#define I2S_GRAY_FLUSH            (0x1u << 2)
#define I2S_GRAY_CLR              (0x1u << 1)
#define I2S_GRAY_EN               (0x1u << 0)

/************************ END I2S DECLARATIONS ********************************/

/************************* BEGIN FUNCTION PROTOTYPES **************************/

/* Char driver functions */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* Software FIFO functions */
static void buffer_init(struct i2s_buffer *b, int32_t *data, int size);
static int32_t buffer_read(struct i2s_buffer *b);
static int buffer_write(struct i2s_buffer *b, int32_t data);
static unsigned int buffer_remaining(struct i2s_buffer *b);
static inline int buffer_items(struct i2s_buffer *b);
static inline void buffer_clear(struct i2s_buffer *b);

/* I2S control functions */
static int i2s_init_default(void);
static void inline i2s_enable(void);
static void inline i2s_disable(void);
static void inline i2s_enable_tx(void);
static void inline i2s_disable_tx(void);
static void inline i2s_enable_rx(void);
static void inline i2s_disable_rx(void);
static void inline i2s_clear_tx_fifo(void);
static void inline i2s_clear_rx_fifo(void);
static void i2s_reset(void);

static irq_handler_t i2s_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs);


/************************* END FUNCTION PROTOTYPES ***************************/

#endif /* I2S_DRIVER_H */
