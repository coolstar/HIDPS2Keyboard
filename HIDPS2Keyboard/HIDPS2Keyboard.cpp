#include "HIDPS2Keyboard.h"
#include "HIDPS2KeyboardWrapper.h"

#define super IOService
OSDefineMetaClassAndStructors(HIDPS2Keyboard, IOService)

typedef struct __attribute__((__packed__)) _HIDPS2_KEYBOARD_REPORT
{
    
    uint8_t      ReportID;
    
    // Left Control, Left Shift, Left Alt, Left GUI
    // Right Control, Right Shift, Right Alt, Right GUI
    uint8_t      ShiftKeyFlags;
    
    uint8_t      Reserved;
    
    // See http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
    // for a list of key codes
    uint8_t      KeyCodes[KBD_KEY_CODES];
    
} HIDPS2KeyboardReport;

typedef struct __attribute__((__packed__)) _HIDPS2_MEDIA_REPORT
{
    
    uint8_t      ReportID;
    
    uint8_t	  ControlCode;
    
    uint8_t	  Reserved;
    
} HIDPS2KeyboardMediaReport;

unsigned char hidps2keyboarddesc[] = {
    //
    // Keyboard report starts here
    //
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                         // USAGE (Keyboard)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_KEYBOARD,            //   REPORT_ID (Keyboard)
    0x05, 0x07,                         //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                         //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                         //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x08,                         //   REPORT_COUNT (8)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x81, 0x03,                         //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                         //   REPORT_COUNT (5)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x05, 0x08,                         //   USAGE_PAGE (LEDs)
    0x19, 0x01,                         //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                         //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                         //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x03,                         //   REPORT_SIZE (3)
    0x91, 0x03,                         //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                         //   REPORT_COUNT (6)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                         //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                         //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                         //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                         //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                         //   INPUT (Data,Ary,Abs)
    0xc0,                               // END_COLLECTION
    
    0x05, 0x0C, /*		Usage Page (Consumer Devices)		*/
    0x09, 0x01, /*		Usage (Consumer Control)			*/
    0xA1, 0x01, /*		Collection (Application)			*/
    0x85, REPORTID_MEDIA,	/*		Report ID=2							*/
    0x05, 0x0C, /*		Usage Page (Consumer Devices)		*/
    0x15, 0x00, /*		Logical Minimum (0)					*/
    0x25, 0x01, /*		Logical Maximum (1)					*/
    0x75, 0x01, /*		Report Size (1)						*/
    0x95, 0x07, /*		Report Count (7)					*/
    0x09, 0x6F, /*		Usage (Brightess Up)				*/
    0x09, 0x70, /*		Usage (Brightness Down)				*/
    0x09, 0xB7, /*		Usage (Stop)						*/
    0x09, 0xCD, /*		Usage (Play / Pause)				*/
    0x09, 0xE2, /*		Usage (Mute)						*/
    0x09, 0xE9, /*		Usage (Volume Up)					*/
    0x09, 0xEA, /*		Usage (Volume Down)					*/
    0x81, 0x02, /*		Input (Data, Variable, Absolute)	*/
    0x95, 0x01, /*		Report Count (1)					*/
    0x81, 0x01, /*		Input (Constant)					*/
    0xC0,        /*        End Collection                        */
};

char* HIDPS2Keyboard::getMatchedName(IOService* provider) {
    OSData *data;
    data = OSDynamicCast(OSData, provider->getProperty("name"));
    return (char*)data->getBytesNoCopy();
}

bool HIDPS2Keyboard::start(IOService * provider){
    if(!super::start(provider))
        return false;
    PMinit();
    
    fACPIDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    
    if (!fACPIDevice)
        return false;
    
    provider = fACPIDevice;
    name = getMatchedName(fACPIDevice);
    
    provider->retain();
    
    if(!provider->open(this)) {
        IOLog("%s::%s::Failed to open ACPI device\n", getName(), name);
        return false;
    }
    
    //provider opened, test here
    workLoop = (IOWorkLoop*)getWorkLoop();
    if(!workLoop) {
        IOLog("%s::%s::Failed to get workloop\n", getName(), name);
        HIDPS2Keyboard::stop(provider);
        return false;
    }
    
    workLoop->retain();
    
    //got workloop, test here
    timerSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &HIDPS2Keyboard::get_input));
    if (!timerSource){
        IOLog("%s", "Timer Err!\n");
        stop(provider);
        return false;
    }
    
    workLoop->addEventSource(timerSource);
    timerSource->setTimeoutMS(10);
    
    IOLog("%s::%s:: Starting!\n", getName(), name);

    initialize_wrapper();
    
    registerService();
    
#define kMyNumberOfStates 2
    
    static IOPMPowerState myPowerStates[kMyNumberOfStates];
    // Zero-fill the structures.
    bzero (myPowerStates, sizeof(myPowerStates));
    // Fill in the information about your device's off state:
    myPowerStates[0].version = 1;
    myPowerStates[0].capabilityFlags = kIOPMPowerOff;
    myPowerStates[0].outputPowerCharacter = kIOPMPowerOff;
    myPowerStates[0].inputPowerRequirement = kIOPMPowerOff;
    // Fill in the information about your device's on state:
    myPowerStates[1].version = 1;
    myPowerStates[1].capabilityFlags = kIOPMPowerOn;
    myPowerStates[1].outputPowerCharacter = kIOPMPowerOn;
    myPowerStates[1].inputPowerRequirement = kIOPMPowerOn;
    
    provider->joinPMtree(this);
    
    registerPowerDriver(this, myPowerStates, kMyNumberOfStates);
    
    return true;
}

void HIDPS2Keyboard::stop(IOService *provider){
    IOLog("%s::stop\n", getName());
    
    destroy_wrapper();
    
    if (timerSource){
        timerSource->cancelTimeout();
        timerSource->release();
        timerSource = NULL;
    }
    
    if (workLoop) {
        workLoop->release();
        workLoop = NULL;
    }

    PMstop();
    
    super::stop(provider);
}

IOReturn HIDPS2Keyboard::setPowerState(unsigned long powerState, IOService *whatDevice){
    if (powerState == 0){
        //Going to sleep
        if (timerSource){
            timerSource->cancelTimeout();
            timerSource->release();
            timerSource = NULL;
        }
        IOLog("::%s::Going to Sleep!\n",getName());
    } else {
        //Waking up from Sleep
        if (!timerSource){
            timerSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &HIDPS2Keyboard::get_input));
            workLoop->addEventSource(timerSource);
            timerSource->setTimeoutMS(10);
        }
        IOLog("::%s::Resuming from Sleep!\n",getName());
    }
   
    return kIOPMAckImplied;
}

void HIDPS2Keyboard::initialize_wrapper(void) {
    destroy_wrapper();
    
    _wrapper = new HIDPS2KeyboardWrapper;
    if (_wrapper->init()) {
        _wrapper->attach(this);
        _wrapper->start(this);
    }
    else {
        _wrapper->release();
        _wrapper = NULL;
    }
}

void HIDPS2Keyboard::destroy_wrapper(void) {
    if (_wrapper != NULL) {
        _wrapper->terminate(kIOServiceRequired | kIOServiceSynchronous);
        _wrapper->release();
        _wrapper = NULL;
    }
}

void HIDPS2Keyboard::get_input(OSObject* owner, IOTimerEventSource* sender) {
    int ps2code = inb(0x60);
    if (ps2code == lastps2code){
        timerSource->setTimeoutMS(10);
        return;
    }
    lastps2code = ps2code;
    keyPressed();
    timerSource->setTimeoutMS(10);
}

void HIDPS2Keyboard::updateSpecialKeys(int ps2code) {
    bool PrepareForRight = PrepareForRight;
    PrepareForRight = false;
    if (ps2code < 0)
        ps2code += 256;
    switch (ps2code) {
        case 29:
            if (PrepareForRight)
                RightCtrl = true;
            else
                LeftCtrl = true;
            return; //ctrl
        case 157:
            if (PrepareForRight)
                RightCtrl = false;
            else
                LeftCtrl = false;
            return; //ctrl
            
        case 56:
            if (PrepareForRight)
                RightAlt = true;
            else
                LeftAlt = true;
            return; //alt
        case 184:
            if (PrepareForRight)
                RightAlt = false;
            else
                LeftAlt = false;
            return; //alt
            
        case 42:
            LeftShift = true;
            return; //left shift
        case 170:
            LeftShift = false;
            return; //left shift
            
        case 54:
            RightShift = true;
            return; //right shift
        case 182:
            RightShift = false;
            return; //right shift
            
        case 91:
            LeftWin = true;
            return; //left win
        case 219:
            LeftWin = false;
            return; //left win
    }
    PrepareForRight = false;
    if (ps2code == 224)
        PrepareForRight = true;
}

uint8_t HIDPS2Keyboard::HIDCodeFromPS2Code(int ps2code, bool *remove) {
    if (ps2code < 0)
        ps2code += 256;
    *remove = false;
    switch (ps2code) {
        case 0x1e:
            return 0x04; //a
        case 158:
            *remove = true;
            return 0x04; //a
            
        case 48:
            return 0x05; //b
        case 176:
            *remove = true;
            return 0x05; //b
            
        case 46:
            return 0x06; //c
        case 174:
            *remove = true;
            return 0x06; //c
            
        case 32:
            return 0x07; //d
        case 160:
            *remove = true;
            return 0x07; //d
            
        case 18:
            return 0x08; //e
        case 146:
            *remove = true;
            return 0x08; //e
            
        case 33:
            return 0x09; //f
        case 161:
            *remove = true;
            return 0x09; //f
            
        case 34:
            return 0x0a; //g
        case 162:
            *remove = true;
            return 0x0a; //g
            
        case 35:
            return 0x0b; //h
        case 163:
            *remove = true;
            return 0x0b; //h
            
        case 23:
            return 0x0c; //i
        case 151:
            *remove = true;
            return 0x0c; //i
            
        case 36:
            return 0x0d; //j
        case 164:
            *remove = true;
            return 0x0d; //j
            
        case 37:
            return 0x0e; //k
        case 165:
            *remove = true;
            return 0x0e; //k
            
        case 38:
            return 0x0f; //l
        case 166:
            *remove = true;
            return 0x0f; //l
            
        case 50:
            return 0x10; //m
        case 178:
            *remove = true;
            return 0x10; //m
            
        case 49:
            return 0x11; //n
        case 177:
            *remove = true;
            return 0x11; //n
            
        case 24:
            return 0x12; //o
        case 152:
            *remove = true;
            return 0x12; //o
            
        case 25:
            return 0x13; //p
        case 153:
            *remove = true;
            return 0x13; //p
            
        case 16:
            return 0x14; //q
        case 144:
            *remove = true;
            return 0x14; //q
            
        case 19:
            return 0x15; //r
        case 147:
            *remove = true;
            return 0x15; //r
            
        case 31:
            return 0x16; //s
        case 159:
            *remove = true;
            return 0x16; //s
            
        case 20:
            return 0x17; //t
        case 148:
            *remove = true;
            return 0x17; //t
            
        case 22:
            return 0x18; //u
        case 150:
            *remove = true;
            return 0x18; //u
            
        case 47:
            return 0x19; //v
        case 175:
            *remove = true;
            return 0x19; //v
            
        case 17:
            return 0x1a; //w
        case 145:
            *remove = true;
            return 0x1a; //w
            
        case 45:
            return 0x1b; //x
        case 173:
            *remove = true;
            return 0x1b; //x
            
        case 21:
            return 0x1c; //y
        case 149:
            *remove = true;
            return 0x1c; //y
            
        case 44:
            return 0x1d; //z
        case 172:
            *remove = true;
            return 0x1d; //z
            
        case 2:
            return 0x1e; //1
        case 130:
            *remove = true;
            return 0x1e; //1
            
        case 3:
            return 0x1f; //2
        case 131:
            *remove = true;
            return 0x1f; //2
            
        case 4:
            return 0x20; //3
        case 132:
            *remove = true;
            return 0x20; //3
            
        case 5:
            return 0x21; //4
        case 133:
            *remove = true;
            return 0x21; //4
            
        case 6:
            return 0x22; //5
        case 134:
            *remove = true;
            return 0x22; //5
            
        case 7:
            return 0x23; //6
        case 135:
            *remove = true;
            return 0x23; //6
            
        case 8:
            return 0x24; //7
        case 136:
            *remove = true;
            return 0x24; //7
            
        case 9:
            return 0x25; //8
        case 137:
            *remove = true;
            return 0x25; //8
            
        case 10:
            return 0x26; //9
        case 138:
            *remove = true;
            return 0x26; //9
            
        case 11:
            return 0x27; //0
        case 139:
            *remove = true;
            return 0x27; //0
            
        case 28:
            return 0x28; //Enter
        case 156:
            *remove = true;
            return 0x28; //Enter
            
        case 1:
            return 0x29; //Escape
        case 129:
            *remove = true;
            return 0x29; //Escape
            
        case 14:
            return 0x2a; //Backspace
        case 142:
            *remove = true;
            return 0x2a; //Backspace
            
        case 15:
            return 0x2b; //Tab
        case 143:
            *remove = true;
            return 0x2b; //Tab
            
        case 57:
            return 0x2c; //Space
        case 185:
            *remove = true;
            return 0x2c; //Space
            
        case 12:
            return 0x2d; //-
        case 140:
            *remove = true;
            return 0x2d; //-
            
        case 13:
            return 0x2e; //=
        case 141:
            *remove = true;
            return 0x2e; //=
            
        case 26:
            return 0x2f; //[
        case 154:
            *remove = true;
            return 0x2f; //[
            
        case 27:
            return 0x30; //]
        case 155:
            *remove = true;
            return 0x30; //]
            
        case 43:
            return 0x31; //|
        case 171:
            *remove = true;
            return 0x31; //|
            
        case 86:
            return 0x64; //Non-US \ and |
        case 214:
            *remove = true;
            return 0x64; //Non-US \ and |
        case 39:
            return 0x33; //;
        case 167:
            *remove = true;
            return 0x33; //;
            
        case 40:
            return 0x34; //'
        case 168:
            *remove = true;
            return 0x34; //'
            
        case 41:
            return 0x35; //`
        case 169:
            *remove = true;
            return 0x35; //`
            
        case 51:
            return 0x36; //,
        case 179:
            *remove = true;
            return 0x36; //,
            
        case 52:
            return 0x37; //.
        case 180:
            *remove = true;
            return 0x37; //.
            
        case 53:
            return 0x38; // /
        case 181:
            *remove = true;
            return 0x38; // /
            
        case 59:
            return 0x3a; // F1
        case 187:
            *remove = true;
            return 0x3a; // F1
            
        case 60:
            return 0x3b; // F2
        case 188:
            *remove = true;
            return 0x3b; // F2
            
        case 61:
            return 0x3c; // F3
        case 189:
            *remove = true;
            return 0x3c; // F3
            
        case 62:
            return 0x3d; // F4
        case 190:
            *remove = true;
            return 0x3d; // F4
            
        case 63:
            return 0x3e; // F5
        case 191:
            *remove = true;
            return 0x3e; // F5
            
        case 64:
            return 0x3f; // F6
        case 192:
            *remove = true;
            return 0x3f; // F6
            
        case 65:
            return 0x40; // F7
        case 193:
            *remove = true;
            return 0x40; // F7
            
        case 66:
            return 0x41; // F8
        case 194:
            *remove = true;
            return 0x41; // F8
            
        case 67:
            return 0x42; // F9
        case 195:
            *remove = true;
            return 0x42; // F9
            
        case 68:
            return 0x43; // F10
        case 196:
            *remove = true;
            return 0x43; // F10
            
        case 77:
            return 0x4f; // right arrow
        case 205:
            *remove = true;
            return 0x4f; // right arrow
            
        case 75:
            return 0x50; // left arrow
        case 203:
            *remove = true;
            return 0x50; // left arrow
            
        case 80:
            return 0x51; // down arrow
        case 208:
            *remove = true;
            return 0x51; // down arrow
            
        case 72:
            return 0x52; // up arrow
        case 200:
            *remove = true;
            return 0x52; // up arrow
    }
    return 0x00;
}

void HIDPS2Keyboard::removeCode(uint8_t code) {
    for (int i = 0; i < KBD_KEY_CODES; i++) {
        if (keyCodes[i] == code) {
            keyCodes[i] = 0x00;
        }
    }
    uint8_t myKeyCodes[KBD_KEY_CODES] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    int j = 0;
    for (int i = 0; i < KBD_KEY_CODES; i++) {
        if (keyCodes[i] != 0x00) {
            myKeyCodes[j] = keyCodes[i];
            j++;
        }
    }
    for (int i = 0; i < KBD_KEY_CODES; i++) {
        keyCodes[i] = myKeyCodes[i];
    }
}

void HIDPS2Keyboard::addCode(uint8_t code) {
    for (int i = 0; i < KBD_KEY_CODES; i++) {
        if (keyCodes[i] == 0x00) {
            keyCodes[i] = code;
            break;
        }
        else if (keyCodes[i] == code) {
            return;
        }
    }
}

void HIDPS2Keyboard::keyPressed() {
    char ps2code = lastps2code;
    bool remove = false;
    uint8_t hidcode = HIDCodeFromPS2Code(ps2code, &remove);
    if (remove) {
        removeCode(hidcode);
    }
    else {
        addCode(hidcode);
    }
    
    updateSpecialKeys(ps2code);
    
    bool overrideCtrl = false;
    bool overrideRCtrl = false;
    bool overrideAlt = false;
    bool overrideAltGr = false;
    bool overrideWin = false; // win = cmd
    bool overrideShift = false;
    bool mediaKey = false;
    
    uint8_t keyScanCodes[KBD_KEY_CODES] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t consumerKey = 0x00;
    
    for (int i = 0; i < KBD_KEY_CODES; i++) {
        keyScanCodes[i] = keyCodes[i];
#if UseChromeLayout
        uint8_t keyCode = keyScanCodes[i];
        if (LeftCtrl) {
            if (keyCode == 0x2a) {
                if (!LeftAlt)
                    overrideCtrl = true;
                keyScanCodes[i] = 0x4c; //delete (backspace)
            }
        } else {
            if (keyCode == 0x3a) {
                overrideWin = true;
                keyScanCodes[i] = 0x2F;
                //Cmd + [ (F1)
            }
            else if (keyCode == 0x3b) {
                overrideWin = true;
                keyScanCodes[i] = 0x30;
                //Cmd + ] (F2)
            }
            else if (keyCode == 0x3c) {
                overrideWin = true;
                keyScanCodes[i] = 0x15;
                //Cmd + R (F3)
            }
            else if (keyCode == 0x3d) {
                overrideWin = true;
                overrideShift = true;
                keyScanCodes[i] = 0x09;
                //Cmd + Shift + F (F4)
            }
            else if (keyCode == 0x3e) {
                overrideCtrl = true;
                keyScanCodes[i] = 0x52;
                //Ctrl + Up (F5)
            }
            else if (keyCode == 0x3f) {
                mediaKey = true;
                consumerKey = 0x02;
                //brightness down (F6)
            }
            else if (keyCode == 0x40) {
                mediaKey = true;
                consumerKey = 0x01;
                //brightness up (F7)
            }
            else if (keyCode == 0x41) {
                mediaKey = true;
                consumerKey = 0x10; //mute (F8)
            }
            else if (keyCode == 0x42) {
                mediaKey = true;
                consumerKey = 0x40; //volume down (F9)
            }
            else if (keyCode == 0x43) {
                mediaKey = true;
                consumerKey = 0x20; //volume up (F10)
            }
        }
#endif
    }
    
    uint8_t ShiftKeys = 0;
    if (LeftCtrl != overrideCtrl)
        ShiftKeys |= KBD_LCONTROL_BIT;
#if REMAPKEYSLIKEVOODOOPS2
    if (LeftAlt != overrideWin)
        ShiftKeys |= KBD_LGUI_BIT;
#else
    if (LeftAlt != overrideAlt)
        ShiftKeys |= KBD_LALT_BIT;
#endif
    if (LeftShift != overrideShift)
        ShiftKeys |= KBD_LSHIFT_BIT;
#if REMAPKEYSLIKEVOODOOPS2
    if (LeftWin != overrideAlt)
        ShiftKeys |= KBD_LALT_BIT;
#else
    if (LeftWin != overrideWin)
        ShiftKeys |= KBD_LGUI_BIT;
#endif
    
    if (RightCtrl != overrideRCtrl)
        ShiftKeys |= KBD_RCONTROL_BIT;
    if (RightAlt != overrideAltGr)
        ShiftKeys |= KBD_RALT_BIT;
    if (RightShift)
        ShiftKeys |= KBD_RSHIFT_BIT;
    
    if (mediaKey){
        update_media_key(consumerKey);
    } else {
        update_media_key(0x00);
        update_keyboard(ShiftKeys, keyScanCodes);
    }
}

void HIDPS2Keyboard::update_media_key(uint8_t consumerKey) {
    _HIDPS2_MEDIA_REPORT report;
    report.ReportID = REPORTID_MEDIA;
    report.ControlCode = consumerKey;
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(report));
    buffer->writeBytes(0, &report, sizeof(report));
    
    IOReturn err = _wrapper->handleReport(buffer, kIOHIDReportTypeInput);
    if (err != kIOReturnSuccess)
        IOLog("Error handling report: 0x%.8x\n", err);
    
    buffer->release();
}

void HIDPS2Keyboard::update_keyboard(uint8_t shiftKeys, uint8_t keyScanCodes[KBD_KEY_CODES]) {
    _HIDPS2_KEYBOARD_REPORT report;
    report.ReportID = REPORTID_KEYBOARD;
    report.ShiftKeyFlags = shiftKeys;
    for (int i = 0; i < KBD_KEY_CODES; i++){
        report.KeyCodes[i] = keyScanCodes[i];
    }
    
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof(report));
    buffer->writeBytes(0, &report, sizeof(report));
    
    IOReturn err = _wrapper->handleReport(buffer, kIOHIDReportTypeInput);
    if (err != kIOReturnSuccess)
        IOLog("Error handling report: 0x%.8x\n", err);
    
    buffer->release();
}


int HIDPS2Keyboard::reportDescriptorLength(){
    return sizeof(hidps2keyboarddesc);
}

int HIDPS2Keyboard::vendorID(){
    return 'ipcA';
}

int HIDPS2Keyboard::productID(){
    return 'k2sP';
}

void HIDPS2Keyboard::write_report_to_buffer(IOMemoryDescriptor *buffer){
    
    _HIDPS2_KEYBOARD_REPORT report;
    report.ReportID = REPORTID_KEYBOARD;
    report.ShiftKeyFlags = 0;
    for (int i = 0; i < KBD_KEY_CODES; i++){
        report.KeyCodes[i] = 0;
    }
    
    UInt rsize = sizeof(report);
    
    buffer->writeBytes(0, &report, rsize);
}

void HIDPS2Keyboard::write_report_descriptor_to_buffer(IOMemoryDescriptor *buffer){
    
    UInt rsize = sizeof(hidps2keyboarddesc);
    
    buffer->writeBytes(0, hidps2keyboarddesc, rsize);
}