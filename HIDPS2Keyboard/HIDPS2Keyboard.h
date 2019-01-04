#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOInterruptEventSource.h>
#include <string.h>
#include <architecture/i386/pio.h>

#define REMAPKEYSLIKEVOODOOPS2 1
#define UseChromeLayout 0

#define kIOPMPowerOff		0

#define REPORTID_KEYBOARD       0x07
#define REPORTID_MEDIA       0x08

//
// Keyboard specific report infomation
//

#define KBD_LCONTROL_BIT     1
#define KBD_LSHIFT_BIT       2
#define KBD_LALT_BIT         4
#define KBD_LGUI_BIT         8
#define KBD_RCONTROL_BIT     16
#define KBD_RSHIFT_BIT       32
#define KBD_RALT_BIT         64
#define KBD_RGUI_BIT         128

#define KBD_KEY_CODES        6

class IOBufferMemoryDescriptor;
class HIDPS2KeyboardWrapper;

class HIDPS2Keyboard : public IOService {
    
    OSDeclareDefaultStructors(HIDPS2Keyboard);

private:
    HIDPS2KeyboardWrapper *_wrapper;
    
    bool PrepareForRight;
    bool LeftCtrl;
    bool RightCtrl;
    bool LeftAlt;
    bool RightAlt;
    bool LeftShift;
    bool RightShift;
    bool LeftWin;
    
    int lastps2code = 0x00;
    
    uint8_t keyCodes[KBD_KEY_CODES];
    
    char *name;
    
    void initialize_wrapper(void);
    void destroy_wrapper(void);
    
public:
    IOACPIPlatformDevice* fACPIDevice;

    IOACPIPlatformDevice *provider;
    
    IOWorkLoop *workLoop;
    IOInterruptEventSource* interruptSource;
    
    char* getMatchedName(IOService* provider);
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual IOReturn setPowerState(unsigned long powerState, IOService *whatDevice) override;
    void get_input(OSObject* owner, IOTimerEventSource* sender);
    
    void keyPressed();
    void updateSpecialKeys(int ps2code);
    uint8_t HIDCodeFromPS2Code(int ps2code, bool *remove);
    void removeCode(uint8_t code);
    void addCode(uint8_t code);
    
    void update_media_key(uint8_t consumerKey);
    void update_keyboard(uint8_t shiftKeys, uint8_t keyCodes[KBD_KEY_CODES]);
    
    int reportDescriptorLength();
    
    int vendorID();
    int productID();
    
    void write_report_to_buffer(IOMemoryDescriptor *buffer);
    void write_report_descriptor_to_buffer(IOMemoryDescriptor *buffer);
};
