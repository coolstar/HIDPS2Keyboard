//
//  HIDPS2KeyboardWrapper.cpp
//  HIDPS2Keyboard
//
//  Created by Christopher Luu on 10/7/15.
//  Copyright © 2015 Alexandre Daoud. All rights reserved.
//

#include "HIDPS2KeyboardWrapper.h"
#include "HIDPS2Keyboard.h"

OSDefineMetaClassAndStructors(HIDPS2KeyboardWrapper, IOHIDDevice)

static HIDPS2Keyboard* GetOwner(const IOService *us)
{
    IOService *prov = us->getProvider();
    
    if (prov == NULL)
        return NULL;
    return OSDynamicCast(HIDPS2Keyboard, prov);
}

bool HIDPS2KeyboardWrapper::start(IOService *provider) {
    if (OSDynamicCast(HIDPS2Keyboard, provider) == NULL)
        return false;

    setProperty("HIDDefaultBehavior", OSString::withCString("Keyboard"));
    return IOHIDDevice::start(provider);
}

IOReturn HIDPS2KeyboardWrapper::setProperties(OSObject *properties) {
    return kIOReturnUnsupported;
}

IOReturn HIDPS2KeyboardWrapper::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, GetOwner(this)->reportDescriptorLength());

    if (buffer == NULL) return kIOReturnNoResources;
    GetOwner(this)->write_report_descriptor_to_buffer(buffer);
    *descriptor = buffer;
    return kIOReturnSuccess;
}

IOReturn HIDPS2KeyboardWrapper::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    return kIOReturnUnsupported;
}

IOReturn HIDPS2KeyboardWrapper::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeOutput){
        GetOwner(this)->write_report_to_buffer(report);
        return kIOReturnSuccess;
    }
    return kIOReturnUnsupported;
}

/*IOReturn HIDPS2KeyboardWrapper::handleReport(
                                        IOMemoryDescriptor * report,
                                        IOHIDReportType      reportType,
                                        IOOptionBits         options  ) {
    return IOHIDDevice::handleReport(report, reportType, options);
}*/

OSString* HIDPS2KeyboardWrapper::newManufacturerString() const {
    return OSString::withCString("HID");
}

OSNumber* HIDPS2KeyboardWrapper::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_GD_Keyboard, 32);
}

OSNumber* HIDPS2KeyboardWrapper::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_GenericDesktop, 32);
}

OSNumber* HIDPS2KeyboardWrapper::newProductIDNumber() const {
    return OSNumber::withNumber(GetOwner(this)->productID(), 32);
}

OSString* HIDPS2KeyboardWrapper::newProductString() const {
    return OSString::withCString("PS2 Keyboard");
}

OSString* HIDPS2KeyboardWrapper::newSerialNumberString() const {
    return OSString::withCString("1234");
}

OSString* HIDPS2KeyboardWrapper::newTransportString() const {
    return OSString::withCString("PS2");
}

OSNumber* HIDPS2KeyboardWrapper::newVendorIDNumber() const {
    return OSNumber::withNumber(GetOwner(this)->vendorID(), 16);
}

OSNumber* HIDPS2KeyboardWrapper::newLocationIDNumber() const {
    return OSNumber::withNumber(123, 32);
}
