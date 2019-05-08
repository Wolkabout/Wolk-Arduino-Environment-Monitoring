#include <FlashStorage.h>

/*Circular buffer to store outbound messages to persist*/
typedef struct{

  boolean valid;

  outbound_message_t outbound_messages[STORAGE_SIZE];

  uint32_t head;
  uint32_t tail;

  boolean empty;
  boolean full;

} Messages;

static Messages data;

/* Init flash storage */
FlashStorage(flash_store, Messages);

/*Flash storage and custom persistence implementation*/
void _flash_store()
{
  data.valid = true;
  flash_store.write(data);
}
void increase_pointer(uint32_t* pointer)
{
    if ((*pointer) == (STORAGE_SIZE - 1))
    {
        (*pointer) = 0;
    }
    else
    {
        (*pointer)++;
    }
}

void init_flash()
{
    data = flash_store.read();

    if (data.valid == false)
    {
      data.head = 0;
      data.tail = 0;

      data.empty = true;
      data.full = false;

    }
}

bool _push(outbound_message_t* outbound_message)
{
    if(data.full)
    {
        increase_pointer(&data.head);
    }

    memcpy(&data.outbound_messages[data.tail], outbound_message, sizeof(outbound_message_t));

    increase_pointer(&data.tail);
    
    data.empty = false;
    data.full = (data.tail == data.head);

    return true;
}

bool _peek(outbound_message_t* outbound_message)
{
    memcpy(outbound_message, &data.outbound_messages[data.head], sizeof(outbound_message_t));
    return true;
}

bool _pop(outbound_message_t* outbound_message)
{
    memcpy(outbound_message, &data.outbound_messages[data.head], sizeof(outbound_message_t));
    
    increase_pointer(&data.head);
    
    data.full = false;
    data.empty = (data.tail == data.head);

    return true;
}

bool _is_empty()
{
    return data.empty;
}
