/*
 * AppleIntelTGLController.h
 *
 * Main driver controller for Intel graphics
 * Ported from Linux i915_driver.c
 */

#ifndef AppleIntelTGLController_h
#define AppleIntelTGLController_h

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include "linux_types.h"

/* PCI Device IDs - Tiger Lake (Gen12 Xe-LP) */
#define PCI_DEVICE_ID_INTEL_TGL_9A49    0x9A49
#define PCI_DEVICE_ID_INTEL_TGL_9A40    0x9A40
#define PCI_DEVICE_ID_INTEL_TGL_9A59    0x9A59
#define PCI_DEVICE_ID_INTEL_TGL_9A60    0x9A60
#define PCI_DEVICE_ID_INTEL_TGL_9A68    0x9A68
#define PCI_DEVICE_ID_INTEL_TGL_9A70    0x9A70
#define PCI_DEVICE_ID_INTEL_TGL_9A78    0x9A78

/* PCI Device IDs - Alder Lake (Gen12 Xe-LP / 12th Gen UHD Graphics) */
#define PCI_DEVICE_ID_INTEL_ADL_468B    0x468B  /* Alder Lake-S UHD Graphics (Primary) */
#define PCI_DEVICE_ID_INTEL_ADL_4680    0x4680
#define PCI_DEVICE_ID_INTEL_ADL_4682    0x4682
#define PCI_DEVICE_ID_INTEL_ADL_4690    0x4690
#define PCI_DEVICE_ID_INTEL_ADL_4692    0x4692
#define PCI_DEVICE_ID_INTEL_ADL_46A6    0x46A6
#define PCI_DEVICE_ID_INTEL_ADL_46A8    0x46A8
#define PCI_DEVICE_ID_INTEL_ADL_46B3    0x46B3
#define PCI_DEVICE_ID_INTEL_ADL_46C0    0x46C0

/* Architecture helpers */
#define IS_TIGERLAKE(devid)   ((devid) >= 0x9a40 && (devid) <= 0x9a78)
#define IS_ALDERLAKE(devid)   ((devid) == PCI_DEVICE_ID_INTEL_ADL_468B || ((devid) >= 0x4680 && (devid) <= 0x46cf))
#define IS_GEN12_XE_LP(devid) (IS_TIGERLAKE(devid) || IS_ALDERLAKE(devid))

// Forward declarations
class IntelPCIDevice;
class IntelDeviceInfo;
class IntelGTT;
class IntelGEM;
class IntelGEMObject;
class IntelUncore;
class IntelRingBuffer;
class IntelContext;
class IntelDisplay;
class IntelPowerManagement;
class IntelGTPowerManagement;
class IntelRuntimePM;
class IntelDisplayInterrupts;
class IntelGTInterrupts;
class IntelGuC;
class IntelGuCSubmission;
class IntelRequestManager;
class IntelIOAccelerator;

class AppleIntelTGLController : public IOService {
    OSDeclareDefaultStructors(AppleIntelTGLController)
    
public:
    /* IOService overrides */
    virtual bool init(OSDictionary *dictionary = NULL) APPLE_KEXT_OVERRIDE;
    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void free(void) APPLE_KEXT_OVERRIDE;

    /* Power Management */
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal,
                                  IOService *whatDevice) APPLE_KEXT_OVERRIDE;

    /* Accessors */
    IOPCIDevice* getPCIDevice() const { return pciDevice; }
    UInt16 getDeviceID() const { return deviceID; }
    UInt16 getVendorID() const { return vendorID; }
    UInt8 getRevisionID() const { return revisionID; }
    IntelUncore* getUncore() const { return uncore; }
    void __iomem* getMMIOBase() const;
    IntelDeviceInfo* getDeviceInfo() const { return deviceInfo; }
    IOWorkLoop* getWorkLoop() const override { return workLoop; }
    IntelGTT* getGTT() const { return gtt; }
    IntelGEM* getGEM() const { return gem; }
    IntelRingBuffer* getRenderRing() const { return renderRing; }
    IntelContext* getDefaultContext() const { return defaultContext; }
    IntelDisplay* getDisplay() const { return display; }
    IntelGTPowerManagement* getGTPower() const { return gtPower; }
    IntelRuntimePM* getRuntimePM() const { return runtimePM; }
    IntelDisplayInterrupts* getDisplayInterrupts() const { return displayInterrupts; }
    IntelGTInterrupts* getGTInterrupts() const { return gtInterrupts; }
    
    // GuC accessors
    IntelGuC* getGuC() const { return guc; }
    IntelGuCSubmission* getGuCSubmission() const { return gucSubmission; }
    IntelPowerManagement* getPowerManagement() const { return powerMgmt; }
    
    // IOAccelerator accessor
    IntelIOAccelerator* getAccelerator() const { return accelerator; }
    
    // Blitter accessor
    class IntelBlitter* getBlitter() const { return blitter; }
    
    // Framebuffer GGTT address
    uint64_t getFramebufferGGTT() const { return framebufferGGTT; }
    void setFramebufferGGTT(uint64_t addr) { framebufferGGTT = addr; }
    
    // Fence management
    class IntelFence* createFence();
    class IntelFence* findFence(uint32_t fenceId);
    void signalFence(uint32_t fenceId);
    void releaseFence(uint32_t fenceId);
    
    /* Register access helpers */
    u32 readRegister32(u32 offset) const;
    void writeRegister32(u32 offset, u32 value);
    
    /* Ring buffer access */
    IntelRingBuffer* getRingBuffer(int ring_id) const;
    
    /* GEM object allocation */
    IntelGEMObject* allocateGEMObject(size_t size);
    
    /* Context management */
    void destroyContext(IntelContext* context);
    
    /* Request management */
    class IntelRequestManager* getRequestManager() const { return requestManager; }
    
    /* IOSurface Integration (Phase 1) */
    IOReturn mapSurfaceToGPU(IOMemoryDescriptor* mem, uint64_t* outGPUAddr);
    IOReturn unmapSurfaceFromGPU(uint64_t gpuAddress);
    IOReturn mapSurfaceToGGTT(IOMemoryDescriptor* mem, uint64_t* outGPUAddr);
    IOReturn unmapSurfaceFromGGTT(uint64_t gpuAddress);
    uint64_t allocateGPUVirtualAddress(uint64_t size);
    IOReturn bindGEMtoPPGTT(IntelGEMObject* gem, IntelContext* context);
    IOReturn unbindGEMfromPPGTT(IntelGEMObject* gem, IntelContext* context);

private:
    /* Hardware detection and initialization */
    bool initPowerManagement();
    bool detectHardware();
    bool initializeHardware();
    bool setupDeviceInfo();
    bool setupUncore();
    bool setupMemoryManagement();
    bool setupGTT();
    bool setupGEM();
    bool setupGuC();
    bool setupRenderRing();
    bool setupBlitter();
    bool setupDefaultContext();
    bool setupDisplay();
    bool setupPowerManagement();

    /* Cleanup */
    void cleanupHardware();
    void cleanupResources();

    /* Device components */
    IOPCIDevice          *pciDevice;
    IntelPCIDevice       *i915PciDev;
    IntelDeviceInfo      *deviceInfo;
    IntelUncore          *uncore;
    IntelGTT             *gtt;
    IntelGEM             *gem;
    IntelRingBuffer      *renderRing;
    IntelContext         *defaultContext;
    IntelDisplay         *display;
    class IntelBlitter   *blitter;
    IntelGTPowerManagement *gtPower;
    IntelRuntimePM       *runtimePM;
    IntelDisplayInterrupts *displayInterrupts;
    IntelGTInterrupts    *gtInterrupts;
    IntelGuC             *guc;
    IntelGuCSubmission   *gucSubmission;
    IntelPowerManagement *powerMgmt;
    IntelRequestManager  *requestManager;
    IntelIOAccelerator   *accelerator;

    /* Fence management */
    OSArray              *activeFences;
    IOLock               *fenceLock;
    uint32_t             nextFenceId;
    
    /* GEM object tracking */
    OSArray              *gemObjects;

    /* Workloop and gates */
    IOWorkLoop           *workLoop;
    IOCommandGate        *commandGate;
    IOTimerEventSource   *watchdogTimer;

    /* Memory regions */
    IOMemoryMap          *mmioMap;
    IOMemoryMap          *gttMap;

    /* Driver state */
    bool                 isInitialized;
    bool                 isResuming;
    UInt16               deviceID;
    UInt16               vendorID;
    UInt8                revisionID;
    uint64_t             framebufferGGTT;

    /* Statistics */
    UInt64               startTime;
    UInt32               resetCount;
};

#endif /* AppleIntelTGLController_h */
