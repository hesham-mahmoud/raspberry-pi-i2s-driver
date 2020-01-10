/* File: i2s_driver.c
 * Author: TR
 * Description: Low level driver for the RPI's I2S peripheral
 * Version History
 * v1.0 Initial release
*/

/* Dependencies */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>       // For file operations
#include <asm/uaccess.h>    // For get/put_user
#include <asm/io.h>
#include <linux/ioport.h>   // For ioremap, request_mem_region
#include <asm/barrier.h>    // For wmb(), rmb()
#include <linux/slab.h>     // For kmalloc

#include <linux/uaccess.h>

#include <linux/sched.h>    // For interrupts
#include <linux/interrupt.h>

#include "i2s_driver.h"

#define BYTES_PER_SAMPLE        4    // Everything is stored in 32 bits
#define SAMPLE_BUFF_LEN         0x1000
#define SAMPLE_BUFF_LEN_MASK    (SAMPLE_BUFF_LEN - 1)
#define SAMPLE_BUFF_LEN_BYTES   BYTES_PER_SAMPLE * SAMPLE_BUFF_LEN

/* Interrupt number associated with the I2S interface */
#define I2S_INTERRUPT 79

MODULE_AUTHOR("TR");
MODULE_DESCRIPTION("Low level drivers for Raspberry Pi's I2S interface.");
MODULE_LICENSE("GPL");

/* Struct pointer to the I2S base address after remapping.
 * Experimenting with volatile, recommended for memory mapped peripherals */
volatile struct i2s_inst *i2s;

/* Variables for driver's major number and usage indication */
static int major;
static int device_in_use = 0;

/* Buffers to hold 8 bit input/output from the user */
// static char rx_byte_buff[4] = {0,0,0,0};
// static char tx_byte_buff[3 * SAMPLE_BUFF_LEN];

/* Buffers to hold samples for the codec */
static struct i2s_buffer rx_buf;
static struct i2s_buffer tx_buf;
static uint32_t rx_buffer[SAMPLE_BUFF_LEN];
static uint32_t tx_buffer[SAMPLE_BUFF_LEN];

/* Keep track of the number of interrupts where an error occurs */
static int tx_error_count = 0;
static int rx_error_count = 0;

static struct file_operations fops = {
  .read  = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release,
  .unlocked_ioctl = device_ioctl
};


/*
 *****************************************
 * Abstracted functions for the I2S interface.
 *****************************************
 */

static int i2s_init_default(void)
{

  /* Make sure memory isn't being used by something else */
  if(request_mem_region(I2S_BASE, I2S_SIZE, DEVICE_NAME) == NULL)
  {
    printk(KERN_ALERT "Failed to request I2S memory.");
    return -EBUSY;
  }

  /* Convert physical addresses into virtual addresses for the kernel to use */
  i2s = (volatile struct i2s_inst *) ioremap(I2S_BASE, I2S_SIZE);
  printk(KERN_INFO "I2S memory acquired successfully.");

  // Initialize software buffers
  buffer_init(&rx_buf, rx_buffer, SAMPLE_BUFF_LEN);
  buffer_init(&tx_buf, tx_buffer, SAMPLE_BUFF_LEN);

  wmb();
  // Clear control registers
  i2s->CS_A = 0;
  i2s->MODE_A = 0;
  i2s->TXC_A = 0;
  i2s->RXC_A = 0;
  i2s->GRAY = 0;

  printk(KERN_INFO "I2S registers reset.");

  /* Begin register configuration */

  // Use the Raspberry Pi as an I2S slave
  printk(KERN_INFO "Configuring Raspberry Pi as I2S slave...");
  i2s->MODE_A = I2S_MODE_A_CLKM | I2S_MODE_A_FSM;

  /* Configure channels and frame width
   * Gives a channel width of 24 bits,
   * First bit of channel 1 is received on the 2nd clock cycle,
   * First bit of channel 2 is received on the 34rd clock cycle */
  printk(KERN_INFO "Setting channel width...");
#if 1
  // here we try 16/16
  i2s->RXC_A = I2S_RXC_A_CH1EN | I2S_RXC_A_CH1POS(1) | I2S_RXC_A_CH1WID(8) | I2S_RXC_A_CH2EN | I2S_RXC_A_CH2POS(17) | I2S_RXC_A_CH2WID(8);
//  i2s->MODE_A |= I2S_MODE_A_FLEN(31);	// does it make a difference?
//  i2s->MODE_A |= I2S_MODE_A_FSLEN(15);	// does it make a difference?
#else
  // here we try 24/24
  i2s->RXC_A = I2S_RXC_A_CH1EN | I2S_RXC_A_CH1POS(0) | I2S_RXC_A_CH1WEX | I2S_RXC_A_CH1WID(0) | I2S_RXC_A_CH2EN | I2S_RXC_A_CH2POS(30) | I2S_RXC_A_CH2WEX | I2S_RXC_A_CH2WID(0);
#endif
  i2s->TXC_A = I2S_TXC_A_CH1EN | I2S_TXC_A_CH1POS(0) | I2S_TXC_A_CH1WEX | I2S_TXC_A_CH1WID(0) | I2S_TXC_A_CH2EN | I2S_TXC_A_CH2POS(32) | I2S_TXC_A_CH2WEX | I2S_TXC_A_CH2WID(0);

  // Disable Standby
  printk(KERN_INFO "Disabling standby...");
  i2s->CS_A |= I2S_CS_A_STBY;

  // Reset FIFOs
  printk(KERN_INFO "Clearing FIFOs...");
  i2s->CS_A |= I2S_CS_A_TXCLR | I2S_CS_A_RXCLR;

  /* Interrupt driven mode */
  /* Interrupt when TX fifo is less than full and RX fifo less than full */
  i2s->CS_A |= I2S_CS_A_TXTHR(0x1) | I2S_CS_A_RXTHR(0x2);
  // Enable TXW and RXR interrupts
  i2s->INTEN_A = I2S_INTEN_A_TXW | I2S_INTEN_A_RXR;

  // Enable the PCM/I2S module
  printk(KERN_INFO "Enabling I2S...");
  i2s->CS_A |= I2S_CS_A_EN;

  printk(KERN_INFO "I2S configuration Complete.");
  // printk(KERN_INFO "I2S CS contents %x", i2s->CS_A);
  // printk(KERN_INFO "I2S MODE contents %x", i2s->MODE_A);
  // printk(KERN_INFO "I2S RXC contents %x", i2s->RXC_A);
  // printk(KERN_INFO "I2S TXC contents %x", i2s->TXC_A);
  // printk(KERN_INFO "I2S INTEN contents %x", i2s->INTEN_A);
  // printk(KERN_INFO "I2S INTSTC contents %x", i2s->INTSTC_A);
  // printk(KERN_INFO "I2S_SET_TXON macro value = %x", I2S_SET_TXON);
  // printk(KERN_INFO "I2S_SET_RXON macro value = %x", I2S_SET_RXON);
  // printk(KERN_INFO "I2S_TX_BUFF_SPACE macro value = %x", I2S_TX_BUFF_SPACE);

  /* I2S driver is now configured, but TX and RX will need to be turned on before data is transferred */

  return 0;
}

static void inline i2s_enable(void)
{
  wmb();
  i2s->CS_A |= I2S_CS_A_EN;
  printk(KERN_INFO "I2S interface enabled.");
}

static void inline i2s_disable(void)
{
  wmb();
  i2s->CS_A &= (~I2S_CS_A_EN);
  printk(KERN_INFO "I2S interface disabled.");
}

static void inline i2s_enable_tx(void)
{
  wmb();
  i2s->CS_A |= I2S_CS_A_TXON;
  // printk(KERN_INFO "TX enabled.");
}

static void inline i2s_disable_tx(void)
{
  wmb();
  i2s->CS_A &= ~I2S_CS_A_TXON;
  // printk(KERN_INFO "TX disabled.");
}

static void inline i2s_enable_rx(void)
{
  wmb();
  i2s->CS_A |= I2S_CS_A_RXON;
  // printk(KERN_INFO "RX enabled.");
}

static void inline i2s_disable_rx(void)
{  wmb();
  i2s->CS_A &= ~I2S_CS_A_RXON;
  // printk(KERN_INFO "RX disabled.");
  // flush the RX FIFO
  /* Read from the FIFO until it is empty */
  while (i2s->CS_A & I2S_CS_A_RXD_MASK)
  {
     if(buffer_remaining(&rx_buf) != 0)
     {
       buffer_write(&rx_buf, i2s->FIFO_A);
     }
  }
}

static void inline i2s_clear_tx_fifo(void)
{
  i2s->CS_A &= ~(I2S_CS_A_EN);  // Has to be disabled to clear
  wmb();
  i2s->CS_A |= I2S_CS_A_TXCLR;  // Will take two I2S clock cycles to actually clear
  wmb();
  i2s->CS_A |= I2S_CS_A_EN;

}

static void inline i2s_clear_rx_fifo(void)
{
  i2s->CS_A &= ~(I2S_CS_A_EN);  // Has to be disabled to clear
  wmb();
  i2s->CS_A |= I2S_CS_A_RXCLR;  // Will take two I2S clock cycles to actually clear
  wmb();
  i2s->CS_A |= I2S_CS_A_EN;
}

static void i2s_reset(void)
{
  /* Return everything to default */
  wmb();
  // Clear control registers
  i2s->CS_A = 0;
  i2s->MODE_A = 0;
  i2s->TXC_A = 0;
  i2s->RXC_A = 0;
  i2s->INTEN_A = 0;
  i2s->INTSTC_A = 0;
  i2s->GRAY = 0;
}

/*
 ************************************************************
 * Read / write functions for the circular FIFO buffers
 * implementation inspired by Stratify Labs code
 * https://stratifylabs.co/embedded%20design%20tips/2013/10/02/Tips-A-FIFO-Buffer-Implementation/
 ************************************************************
 */

 /* Create a buffer */
 static void buffer_init(struct i2s_buffer *b, int32_t *data, int size)
 {
   b->head = 0;
   b->tail = 0;
   b->size = size;
   b->buffer = data;
 }

 /* Read a single sample from a buffer */
 static int32_t buffer_read(struct i2s_buffer *b)
 {
   int32_t temp;

   if(b->tail != b->head)
   {
     temp = b->buffer[b->tail & SAMPLE_BUFF_LEN_MASK];          // Read sample from the buffer
     smp_wmb();
     b->tail++;                         // Increment tail
   }
   else
   {
     return -1;
   }
   return temp;
 }

 /* Write a sample to a buffer */
 static int buffer_write(struct i2s_buffer *b, int32_t data)
 {
   if(b->head + 1 == b->tail)
   {
     return -1;   //No room
   }
   else
   {
     b->buffer[b->head & SAMPLE_BUFF_LEN_MASK] = data;
     smp_wmb();
     b->head++;
   }
   return 0;
 }

 /* Return the space left in the buffer */
 static unsigned int buffer_remaining(struct i2s_buffer *b)
 {
   return b->size - (b->head - b->tail);
 }

 /* Return number of items currently in the buffer */
 static inline int buffer_items(struct i2s_buffer *b)
 {
   return b->head - b->tail;
 }

 /* Erase contents of buffer */
 static inline void buffer_clear(struct i2s_buffer *b)
 {
   b->head = 0;
   b->tail = 0;
 }

/*
 *************************************************************
 * Interrupt handler
 *************************************************************
 */
static irq_handler_t i2s_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
   // Do interrupt things here
   int i;
   unsigned long irq_flags = 0;
   int32_t data, temp;

   local_irq_save(irq_flags);

   // printk(KERN_ALERT "I2S interrupt fired!...\n");

   /* Check TXW to see if samples can be written/buffer empty. */
   if((i2s->INTSTC_A & I2S_INTSTC_A_TXW))
   {
      for(i = 0; i < 64; i++)
      {
        /* Fill buffer until it is full or until the kernel buffer is empty */
        if(!(i2s->CS_A & I2S_CS_A_TXD_MASK))
        {
          break;
        }

        data = buffer_read(&tx_buf);
        wmb();
        i2s->FIFO_A = data;

        if(buffer_items(&tx_buf) == 0)
        {
          tx_error_count++;
          // printk(KERN_INFO "TX buffer underflow.");
          if(tx_error_count > 1000000)
          {
            /* Shut down to keep from hanging */
            printk(KERN_ALERT "Buffer underflow limit reached. Disabling TX...");
            tx_error_count = 0;
            i2s_disable_tx();

            /* Write a set of samples to stop interrupts */
            i2s->FIFO_A = 0;
            wmb();
            i2s->FIFO_A = 0;
          }
          break;
        }
      }
   }

   /* Check RXR to see if samples have been received and copy them */
   if((i2s->INTSTC_A & I2S_INTSTC_A_RXR))
   {
     /* Read from the FIFO until it is empty */
     while (i2s->CS_A & I2S_CS_A_RXD_MASK)
     {
       if(buffer_remaining(&rx_buf) == 0)
       {
         rx_error_count++;
         // printk(KERN_INFO "RX buffer overflow.");
         if(rx_error_count > 1000000)
         {
           /* Shut it down to keep from hanging forever */
           printk(KERN_ALERT "Buffer overflow limit reached. Disabling RX...");
           rx_error_count = 0;
           i2s_disable_rx();

           /* Read a pair of samples to stop the interrupts */
           wmb();
           temp = i2s->FIFO_A;
           wmb();
           temp = i2s->FIFO_A;

          //  buffer_clear(&rx_buf);
         }
         break;
       }

       rmb();
       buffer_write(&rx_buf, i2s->FIFO_A);
     }
   }

   // Clear all flags
   i2s->INTSTC_A = 0x0F;

   local_irq_restore(irq_flags);
   return (irq_handler_t) IRQ_HANDLED;                      // Announce IRQ has been correctly handled
}

/*
 ************************************************
 * File operation functions for character drivers.
 ************************************************
 */

/* Function to read from the hardware FIFO and transfer data to user space */
static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
  int samples_read = 0;
  int32_t rx_temp;
  unsigned long ret;

  /* Copy data from the I2S FIFO into a user provided buffer
  */

  if(buffer_items(&rx_buf) == 0)
  {
    return 0;
  }

  // Loop until out of samples or length is 0
  while(buffer_items(&rx_buf) && length)
  {
    rx_temp = buffer_read(&rx_buf);

    // Copy 32 bits at a time
    ret = copy_to_user(buffer + (BYTES_PER_SAMPLE * samples_read), &rx_temp, BYTES_PER_SAMPLE);

    // Make sure to decrement by the right amount for 32 bit transfers
    length -= BYTES_PER_SAMPLE;
    samples_read++;

  }

  return BYTES_PER_SAMPLE*samples_read;   // Number of bytes transferred

}

/* Function to write data from user space to the hardware FIFO */
static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
  /* Copy data from user input into the I2S FIFO */

  int i;
  int index = 0;
  unsigned long ret;
  int32_t tx_temp = 0;

   if(buffer_remaining(&tx_buf) == 0)
   {
     // No space available right now
     return -EAGAIN;
   }

  /* Need to convert length from bytes to samples */
  for(i = 0; i < (length / BYTES_PER_SAMPLE); i++)
  {
    // New way that matches the read function more closely
    ret = copy_from_user(&tx_temp, buffer + index, BYTES_PER_SAMPLE);

    if(buffer_write(&tx_buf, tx_temp) < 0)
    {
      printk(KERN_INFO "TX buffer overflow.");
    }

    index += BYTES_PER_SAMPLE;
  }

  // Return the number of bytes transferred
  return BYTES_PER_SAMPLE*index;

}

/* Called when a process attemps to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
  int result;

  if(device_in_use)
  {
    return -EBUSY;
  }

  device_in_use++;

  /* Make sure errors are cleared from any previous use */
  tx_error_count = 0;
  rx_error_count = 0;

  /* Activate interrupts when file is opened */
  result = request_irq(I2S_INTERRUPT, (irq_handler_t) i2s_interrupt_handler, IRQF_TRIGGER_RISING, DEVICE_NAME, NULL);
  if(result < 0)
  {
    printk(KERN_ALERT "Failed to acquire I2S interrupt %d. Returned %d", I2S_INTERRUPT, result);
    return -EBUSY;
  }

  printk(KERN_INFO "I2S interrupts enabled.");

  /* Increment usage count to be able to prooperly close the module. */
  try_module_get(THIS_MODULE);

  return 0;

}

/* Called when the a process closes the device file */
static int device_release(struct inode *inode, struct file *file)
{

  device_in_use--;    /* Make this device available to other processes */

  /* Release the interrupt when the file is closed */
  free_irq(I2S_INTERRUPT, NULL);

  /* Reset errors */
  tx_error_count = 0;
  rx_error_count = 0;

  /* Decrement usage count to be able to properly close the module. */
  module_put(THIS_MODULE);

  return 0;

}

/*
 *********************************************
 * IOCTL function
 *********************************************
 */
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  switch(cmd)
  {
    case I2S_TX_BUFF_SPACE:
      /* Return the number of open spaces left in the buffer */
      return buffer_remaining(&tx_buf);
      break;

    case I2S_RX_BUFF_ITEMS:
      /* Return number of samples in the buffer */
      return buffer_items(&rx_buf);
      break;

    case I2S_CLR_RX_FIFO:
      i2s_clear_rx_fifo();
      break;

    case I2S_CLR_TX_FIFO:
      i2s_clear_tx_fifo();
      break;

    case I2S_SET_EN:
      if(arg == 0)
      {
        i2s_disable();
      }
      else if(arg == 1)
      {
        i2s_enable();
      }
      else
      {
        return -EINVAL;
      }
      break;

    case I2S_SET_TXON:
      if(arg == 0)
      {
        i2s_disable_tx();
      }
      else if(arg == 1)
      {
        i2s_enable_tx();
      }
      else
      {
        return -EINVAL;
      }
      break;

    case I2S_SET_RXON:
      if(arg == 0)
      {
        i2s_disable_rx();
      }
      else if(arg == 1)
      {
        i2s_enable_rx();
      }
      else
      {
        return -EINVAL;
      }
      break;

    case I2S_CLEAR_TX_BUFF:
      buffer_clear(&tx_buf);
      break;

    case I2S_CLEAR_RX_BUFF:
      buffer_clear(&rx_buf);
      break;

    case I2S_WRITE_CS_A:
      wmb();
      i2s->CS_A = arg;
      break;

    case I2S_WRITE_MODE_A:
      wmb();
      i2s->MODE_A = arg;
      break;

    case I2S_WRITE_RXC_A:
      wmb();
      i2s->RXC_A = arg;
      break;

    case I2S_WRITE_TXC_A:
      wmb();
      i2s->TXC_A = arg;

    case I2S_WRITE_DREQ_A:
      wmb();
      i2s->DREQ_A = arg;
      break;

    case I2S_WRITE_INTEN_A:
      wmb();
      i2s->INTEN_A = arg;

    case I2S_WRITE_INTSTC_A:
      wmb();
      i2s->INTSTC_A = arg;
      break;

    case I2S_WRITE_GRAY:
      wmb();
      i2s->GRAY = arg;
      break;

    default:
      return -EINVAL;
  }

  return 0;
}

/*
 **************************************
 *Required module functions functions
 **************************************
 */

/* Function called upon loading module */
int init_module(void)
{
  int status = 1;
  printk(KERN_INFO "Installing I2S driver...");

  //Temporarily use my default settings.
  status = i2s_init_default();

  /* Hardware should now be set up and ready to go */
  if(status < 0)
  {
    printk(KERN_ALERT "Hardware configuration failed. Driver not installed.");
    return -1;
  }

  major = register_chrdev(0, DEVICE_NAME, &fops);
  if(major < 0)
  {
    printk(KERN_ALERT "register_chrdev failed with major = %d\n", major);
    return major;
  }

  printk(KERN_INFO "I2S driver successfully assigned to major number %d.", major);

  return 0;

}

/* Function called upon uninstalling module */
void cleanup_module(void)
{
  /* Disable all the things */
  // printk(KERN_INFO "Disabling I2S interrupts.");
  i2s_disable_tx();
  i2s_reset();
  // printk(KERN_INFO "I2S interface disabled and reset.");

  /* Unmap all memory regions used */
  // printk(KERN_INFO "Unmapping memory regions used.");
  iounmap(i2s);
  release_mem_region(I2S_BASE, I2S_SIZE);

  unregister_chrdev(major, DEVICE_NAME);

  printk(KERN_INFO "I2S driver removed.");
}
