//	UPKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the UP keyin


/*  FORMATS
There are many formats to the UP keyin. You can specify a particular component,
specify a path, or use an option with or without a particular component.
The following keyins specify a component:
    UP ip
    Adds the specified instruction processor to the partition.
    UP iop
    Adds the specified IOP to the partition.
    UP xiipname
    Adds the specified XIIP to the partition.
    UP xpcname
    Adds the specified XPC to the partition.
    UP mmod
    Adds the specified memory module to the partition.
    UP cmod
    Adds the specified channel module to the partition.
    UP cu
    Adds the specified control unit to the partition.
    UP devnam
    Adds the specified device to the partition.

The following keyin specifies an interface:
    UP cmod/[intf#]/cu
    Makes the interface from the specified channel module to the control unit available.
    If you do not specify an interface number, the Exec supplies a value. Also use this
    keyin for bringing up an interface to the record lock processor (RLP).

The following keyins specify an option:
UP,ALL cu
    Adds all devices attached to the specified control unit to the partition.

UP,PACK pack-id[/[dir-id]// [pack-id]...]
    Places the specified packs in the UP state.
    Note: To use the UP,PACK keyin in a local environment, the NEWCON
    configuration parameter REQNBRPST0 must be greater than 0. To use the UP,PACK
    keyin in a shared environment, the NEWCON configuration parameter REQNBRPST1
    must be greater than 0. See the Exec Installation and Configuration Guide for more
    information on these configuration parameters.

UP,XCACHE xpcname
UP,XCACHE xpcname/LOCAL
UP,XCACHE xpcname/SHARED
    Enables caching with the specified XPC (xpcname).
    Use the UP,XCACHE xpcname keyin to enable local and shared caching for each host
    using the XPC.
    Use the UP,XCACHE xpcname/LOCAL keyin to enable a host's local caching using
    the XPC. If local caching was disabled using the DN,XCACHE xpcname/LOCAL
    keyin, local caching can only be enabled using an UP,XCACHE xpcname/LOCAL
    keyin.
    Use the UP,XCACHE xpcname/SHARED keyin to enable a host's shared caching
    using the XPC. Once specified, shared caching can be disabled using a DN,XCACHE
    xpcname/SHARED keyin or a DN,XCACHE xpcname keyin. The UP,XCACHE
    xpcname/SHARED keyin can be issued when the XPC is in MHM mode.
UP,MHM xpcname
    Initializes the XPC for use in a multihost environment.

Remember, when you bring up a fixed disk, you initialize the disk and that initialization
erases all disk files.

Before the system can bring up a control unit, it must enable all usable interfaces to the
control unit. If the system cannot enable all the interfaces, it displays this message:
ENABLE OF channel/module/cu INTERFACE FAILED
Bring down the failing interface and bring up the control unit again.

Bringing up a disk that is prepped as removable causes the files on that disk to be
registered with the system.

If your system uses the Tape Automatic Volume Recognition (TAVR) feature, do not
perform the UP keyin on a tape unit with a mounted reel when the MODIFY CONFIG
message appears. Wait until the SYS START message is complete. If you try to
premount a tape before this point, the system may display this message:
device [ipnam]/iopnam/cmname/cuname NOTRDY REWI run-id A,B,M
Answer B to this message.
Bring the reel to load point before you attempt input to or output from the tape unit. If
you attempt input or output to the tape first, the system displays this message:
tape-unit IS SELECTED
The FS keyin shows the identifier of the reel on the mounted tape, but it does not show
the status as mounted (MNTD).
*/

/*  MESSAGES

    UP component INITIALIZES AND ADDS device TO FIXED MS, PROCESS? YN
    (Exec) Either an UP keyin was performed on a nondisk mass storage device in
    a DN state or an UP keyin was performed on a component, which will cause a
    nondisk device that was unavailable to UP to become available.

    UP KEYIN ABORTED DUE TO STORAGE FAULTS
    (Exec) During the testing of storage for an UP keyin, a storage fault was
    encountered that could not be handled, so the UP keyin has been aborted.

    UP KEYIN - (ALL) OPTION INVALID FOR component,INPUT IGNORED
    (Exec) You cannot perform an UP,ALL keyin on the specified component.

    UP KEYIN - ALREADY PERFORMED FOR component
    (Exec) The specified system component is already up.

    UP KEYIN FAILED - I/O ERROR PROCESSING KEYIN FOR pack-id
    (Exec) An I/O error occurred while processing the keyin; the keyin will be ignored.

    UP KEYIN NOT ALLOWED - ARBITRARY DEVICE HANDLER IN USE BY run-id ON device
    (Exec) An attempt was made to change the status of a device under the control
    of the arbitrary device handler (ADH). The ADH is normally used by a maintenance
    or diagnostic program such as DPREP1100 or by a printer control program. The
    program must release the device before its status can be changed.

    UP OF component NOT ALLOWED UNTIL AFTER FILE RECOVERY
    (Exec) You tried to bring up a component during file recovery. Bringing up the IOP,
    channel modules, or control unit is not allowed until after the file recovery process
    is complete.

    UP OF iop-name FAILED DUE TO AN INTERNAL IOP ERROR
    (Exec) A request to bring up an IOP has failed.

    UP OF component FAILED DUE TO INTERNAL DATA CORRUPTION
    (Exec) Data in the Exec internal memory table has been damaged or destroyed.
    cu-interface is the name of the control unit interface that is being brought up.
    spcx is the SPC on which the error occurred.
    yy is the SPC line on spcx that could not be enabled.

    UP OF component FAILED - ERRORS OCCURRED ACCESSING THE MCT
    (Exec) I/O errors occurred after the UP keyin to the component. component is
    the name of the control unit, IOP, or control unit interface specified on the UP keyin.
    The master configuration table (MCT) could not be read. The line subsystem power
    controller (SPC) numbers that needed to be enabled could not be accessed. The
    control unit is taken offline and the keyin is aborted.

    UP OF cu-interface FAILED: UNABLE TO ACQUIRE PATHS TO TAPE UNITS
    (Exec) No paths to the tape units can be established. The keyin is aborted if this
    error is detected. The variable cu-interface contains the interface name given on
    the UP keyin.

    UP OF tape-unit FAILED: UNABLE TO ACQUIRE THE UNIT
    (Exec) The system is unable to establish a path to the tape unit or the tape unit
    cannot be acquired for some reason other than already being exclusively acquired
    on another system.
    The keyin is aborted if this error condition is detected. If the keyin completes
    successfully, the LED on the tape unit is changed to display the system-id of the
    acquiring system.
    The variable tape-unit contains the name of the U40 cartridge tape unit given on
    the UP keyin.

    UP OF component INITIATED INTERNALLY
    (Exec) An internal request for component partitioning is being processed. Internal
    partitioning requests can be generated through the System Control Facility (SCF) or
    from a hardware fault.

    UP OF component IS NOT ALLOWED
    (Exec) The equipment type is incompatible with the keyin.

    UP OF component NOT PERFORMED - KEYIN ABORTED
    (Exec) You responded N to a previous message to perform an UP keyin.

    UP OF component NOT PERFORMED - NO PATH AVAILABLE
    (Exec) The component cannot be brought up because there is no path available.
    (This applies to components at or higher than the device level.) Call the site
    administrator.

    UP OF device NOT PERFORMED - PACK VERIFICATION ACTIVE
    (Exec) Pack verification is being performed in the disk you are trying to bring
    down or reserve.

    UP of component requested by initiator failed because of an error accessing
    the Master Configuration Table. Notify the Site Administrator.
    (Exec) The partitioning request failed due to an error accessing the master
    configuration table (MCT). The variable initiator is the partitioning requestor (fault
    recovery, SCF, or the operator).

    UP of component requested by initiator was received.
    (Exec) An internal request for component partitioning is being processed. The
    variable initiator identifies who initiated the partitioning request (that is, the operator,
    SCF, fault recovery, or the Exec).
*/



#include	"execlib.h"



//	private / protected methods

//	handler()
//
//	Called from Keyin base class worker() function
void
    UPKeyin::handler()
{
}


//  isAllowed()
bool
    UPKeyin::isAllowed() const
{
    Exec::Status execStatus = m_pExec->getStatus();
    return (execStatus == Exec::ST_BOOTING_1) || (execStatus == Exec::ST_RUNNING);
}



// constructors / destructors

UPKeyin::UPKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:FacilitiesKeyin( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods

