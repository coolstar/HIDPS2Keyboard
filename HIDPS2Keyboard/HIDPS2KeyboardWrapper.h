//
//  HIDPS2KeyboardWrapper.h
//  HIDPS2Keyboard
//
//  Created by Christopher Luu on 10/7/15.
//  Copyright © 2015 Alexandre Daoud. All rights reserved.
//

#ifndef HIDPS2KeyboardWrapper_h
#define HIDPS2KeyboardWrapper_h

#include <IOKit/hid/IOHIDDevice.h>

class HIDPS2KeyboardWrapper : public IOHIDDevice
{
    OSDeclareDefaultStructors(HIDPS2KeyboardWrapper)

public:
    virtual bool start(IOService *provider);
    
    virtual IOReturn setProperties(OSObject *properties);
    
    virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const;
    
    virtual IOReturn setReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options=0);
    virtual IOReturn getReport(IOMemoryDescriptor *report,IOHIDReportType reportType,IOOptionBits options);
    
    virtual OSString* newManufacturerString() const;
    virtual OSNumber* newPrimaryUsageNumber() const;
    virtual OSNumber* newPrimaryUsagePageNumber() const;
    virtual OSNumber* newProductIDNumber() const;
    virtual OSString* newProductString() const;
    virtual OSString* newSerialNumberString() const;
    virtual OSString* newTransportString() const;
    virtual OSNumber* newVendorIDNumber() const;
    
    virtual OSNumber* newLocationIDNumber() const;
};

#endif /* HIDPS2KeyboardWrapper_h */
