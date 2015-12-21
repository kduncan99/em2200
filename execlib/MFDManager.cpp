//  MFDManager class implementation.
//  Copyright (c) 2015 by Kurt Duncan
//
//  Manages the Master File Directory
//
//  Some interesting file names:
//      SYS$*GENF$
//      SYS$*MFDF$$
//      SYS$*LIB$
//      SYS$*DLOC$
//
//  Data types and structures
//      Bit naming - for the information below, the convention is to refer to the MSB as bit 0,
//      and the LSB as bit 35.
//
//  Device-relative word address (DRWA):
//      Refers to the first word of an area of storage (of any length) on some fixed storage medium.
//      Identified by the distance from the beginning of the storage medium, in words.
//
//  Directory Sector Address (DSADDR):
//      30 bits enclosed in a 32-bit integer, with MSBits set 0.
//      The offset of a particular directory sector within the addressing context of the fixed MFD,
//      or identifying a directory sector within a particular removable pack's MFD.
//      0ddddttttss
//          dddd:   For fixed: LDAT index - corresponds to a fixed disk unit, mapped through LDAT (owned by MFD)
//                  For Removable:
//                      Indicates a particular pack mapped through an MFD pack table
//                      In some contexts, it *might* be clear that we are speaking about a particular pack's
//                          MFD, in which case dddd might be ignored, and might be zero.
//          tttt:   Index of a particular MFD track on a given fixed or removable pack -
//                      mapped through the DAD table to an absolute track address for the pack.
//          ss:     sector offset from start of containing track, 0 to 077.
//
//  Directory Link Address:
//      May be used as the first word in a directory sector, to indicate two separate pieces of information.
//          In this case, the lower 5 bits of the link field (S1) indicate the nature of the containing
//          directory sector (is it a lead item, search item, etc), while the MSBit indicates whether the
//          containing directory sector address (bits 6-35) is valid - if MSBit is set, the dsa is zero.
//          If BSBit is clear, the dsa refers to some related directory sector.
//      May be used purely as a link to a related directory sector, in which case MSBit set indicates no link
//          (dsa is zero), while MSBit clear indicates dsa is valid, referring to a related sector.
//          In this case, the remaining bits of the link field have no meaning, and are zero.
//
//  Directory Allocation Sector (DAS):
//      Directory sectors are marked as allocated/unallocated in DAS's, which themselves are directory sectors,
//      and which always account for themselves.  A DAS can account for the sectors in up to 9 consecutive
//      directory track.  Therefore, the first sector of every ninth directory track (beginning with directory
//      track 0) is a DAS.
//      000:H1: LDAT index - since we have only one datastore, this is always zero
//          H2: always zero
//      001:    Allocation bits for 1st 32 sectors of the track containing this DAS
//                  (bit0 corresponds to the first sector in track - which is this DAS)
//                  (bit31 corresponds to the 32nd sector in the track)
//                  bits32-35 are always zero.
//      002:    Allocation bits for last 32 sectors of the track containing this DAS
//                  (bit0 corresponds to the 33rd sector in the track)
//                  (bit31 corresponds to the 61st sector in the track)
//      003:    3-word entry for the next corresponding track (track 1)
//              003:    DRWA of the datastore track containing the MFD track.
//                      If 0400000000000 then the corresponding MFD track is unallocated
//              004:    Bit mask (similar to word 001) for 1st 32 sectors of the directory track
//                          Note that this track does not need, nor have, a DAS
//              005:    Bit mask (similar to word 002) for last 32 sectors of the directory track
//      030:    3-word entry for the last corresponding track (track 8)
//      033:    DRWA of next directory track which contains a DAS.  If 0400000000000, this is the last DAS.
//
//  MFD Sector 0:
//      If +0,S1 is zero, then sector 0 is a DAS for the first (up to) 9 directory tracks.
//      For larger disks, sector 0 is as follows:
//      000,S1: Number of tracks mapped at prep time
//      033:    Device-Relative Word Address of the first DAS track
//
//  MFD Sector 1:
//      This is the 2nd sector of the MFD on any disk device
//      000:    Address of hardware MBT.
//      001:    Address of software MBT.
//      002:    Maximum available tracks, not including label and initial directory track allocation
//      003:    Current available tracks
//      004:    PackId in fieldata
//      005:    H1  LDAT index if this is a fixed pack (or 0400000 for freshly prepped fixed pack)
//                      zero if this is removable
//              H2  MBT length
//      006     S1: Flags - mostly to do with caching and XPC - we don't do any of this, so this is always zero.
//              S2: SDT index - always zero
//              S3: Reserved (zero)
//              S4: Reserved (zero)
//              T3: Prep Status
//                  Bit 31: Alternate track prepped (always clear)
//                  Bit 32: Not alternate track prepped (always set)
//                  Bit 33: Written to by the Exec (always set)
//                  Bit 34: Standard-size prepped (always set)
//                  Bit 35: Alternate tracks not available (always set)
//      007:    Pack registration timestamp - only for removable
//      010:    T1: Physical records per track
//              S3: Sector 1 Version - always 1, to accomodate large devices (which we likely don't need, but...)
//              T3: Words per physical record
//      011:    Reserved
//      012:    Reserved
//      013:    Reserved
//      014:    Cache control device name - unused
//      015:    Address of area reserved for cache tables - unused
//      016:    Address of Unit Duplication resynchronization tables - unused
//      017:    Logical name of Unit-Duplexed partner - unused
//      020:    H1: index into removable pack table for removable packs
//              H2: DAS offset - If zero, the first DAS is in the sector immediately preceding this one
//                      Otherwise, The offset field contains the number of sectors included in
//                      S0/S1/HMBT/padding/SMBT/padding to next 9-track boundary
//      021:    XPC cache info - unused
//      022:    Link to first file name change item, if file name change is in progress
//      023:    Reserved
//      ...
//      033:    Reserved
//
//  Search Item:
//      There is an internal table which uses a hash of a file's qualifier and filename as an index
//      into a vector of DSADDRs, which link to the search item (as described below) on a chain of such
//      items which all have the same hash index.
//      000:    Bit0:   U...if clear, Bits 4-35 are the DSADDR of the next search item with the same hash index
//                          if set, this is the last (or only) search item for the hash index
//              Bit1:   clear
//              Bit2:   clear
//              Bit3:   set
//              Bits6-35:   link to next search item with the same hash index if U==0
//      001:    First entry
//              001:    2-word Fieldata qualifier
//              003:    2-word Fieldata filename
//              005:    link to lead item for the indicated qualifier/filename combination
//      006:    Second entry, or all zeros
//      013:    Third entry, or all zeros
//      020:    Fourth entry, or all zeros
//      025:    Fifth entry, or all zeros
//
//  Lead Item Sector 0:
//      Represents general information about a set of filecycles which have a common qualifier and file name.
//      000:    Bit0:   U...if clear, Bits 4-35 are the DSADDR of lead item sector 1
//                          if set, there is no lead item sector 1
//              Bit1:   clear
//              Bit2:   set
//              Bit3:   clear
//              Bit6-35:link to lead item sector 1 if U==0
//      001:    2-word fieldata qualifier
//      003:    2-word fieldata filename
//      005:    2-word fieldata project ID
//      007:    fieldata read key or zero
//      010:    fieldata write key or zero
//      011:    S1: file type, 000-mass storage, 001=mag tape, 040=removable disk(not used)
//              S2: number of file cycles which actually exist; does not include to-be-dropped or to-be-cataloged
//              S3: Max range of cycles permitted for this file set
//              Q3: current range of f-cycles
//              Q4: highest absolute f-cycle
//      012:    T1: status bits
//                  Bit0:   guarded file (g-option)
//                  Bit1:   A +1 f-cycle currently exists, link is in word 013 (11dec)
//                  Bit2:   file name change is in progress for this file set
//              S3: shared file index (always 0)
//              Q3: number of security words (always 0)
//              Q4: access type (ACR stuff - always 0 for us)
//      013:    Link to main item of highest absolute file cycle
//              Bit0:   File is in a to-be state
//              Bit1:   to-be-cataloged (asg,c)
//              Bit2:   to-be-dropped
//              Bit4-35:link to main item sector 1 if file cycle exists, else 0
//      014:    Link to main item of next-highest absolute file cycle or zero (entry 2)
//      ...
//      033:    Link to main item of file cycle or zero (entry 17)
//
//  Lead Item Sector 1:
//  If it exists, it references additional main items beyond what will fit in sector 0
//      000:    Bit0:   U (always set, since there are no further lead item sectors)
//              Bit1:   clear
//              Bit2:   clear
//              Bit3:   clear
//              Bit4-35:clear
//      001:    Link to main item of file cycle or zero (entry 18)
//      ...
//      017:    Link to main item of file cycle or zero (entry 32)
//      020:    Reserved
//      ...
//      033:    Reserved
//
//  Mass Storage File Main Item Sector 0:
//      000:    Bit0:   U clear if there is a DAD table, else set
//              Bit1:   always set
//              Bit2:   always clear
//              Bit3:   always clear
//              Bit4-35:Link to first DAD table if U==0
//      001:    2-word fieldata qualifier
//      003:    2-word fieldata filename
//      005:    2-word fieldata project ID
//      007:    2-word fieldata account number
//      011:    Reserved
//      012:    Time of 1st write after unload or backup item (TDATE$)
//      013:    S1: Disable flags
//                  Bit0:   Disabled
//                  Bit1:   Directory Error
//                  Bit2:   File written to before system stop
//                  Bit3:   Inaccessible backup
//                  Bit4:   Cache drain failure
//              Bit5-35:    Link to Lead Item
//      014:    T1: Descriptor Flags
//                  Bit0:   Unloaded
//                  Bit1:   Backed up
//                  Bit2:   Save on Checkpoint
//                  Bit3:   Reserved
//                  Bit4:   Reserved
//                  Bit5:   To be cataloged
//                  Bit6:   Tape File
//                  Bit7:   For REM files - extension sectors may have been updated in main MFD (we don't use this)
//                  Bit8:   Removable (REM) file
//                  Bit9:   To be made write-only
//                  Bit10:  To be made read-only
//                  Bit11:  To be dropped when assign count becomes zero
//              S3: File Flags
//                  Bit12:  Large file - affects semantics of highest track written word 027:H1
//                  Bit13:  Reserved
//                  Bit14:  Reserved
//                  Bit15:  Assignment acceleration (we don't use this)
//                  Bit16:  Written to
//                  Bit17:  Store-through (cataloged with S option)
//              H2: Reserved
//      015:    S1: PCHAR flags
//                  Bit0:   Position granularity
//                  Bit1:   Reserved
//                  Bit2:   Word-addressable
//                  Bit3:   File is assigned to a common-name section
//                  Bit4:   Out of sync (we don't use this)
//                  Bit5:   Resynchronization may be required (we don't use this)
//              Bit6-35:    Link to Main Item sector 1
//      016:    Assign mnemonic
//      017:    H1: Link to initial SMOQUE entry
//              H2: Cumulative assign count
//      020:    Reserved
//      021:    S1: Shared file index (always zero)
//              S2: Inhibit flags
//                  Bit6:   Guarded file (option G)
//                  Bit7:   Unload inhibit (option V)
//                  Bit8:   Private file (option P not present on cat or asg)
//                  Bit9:   Exclusive use (asg,X)
//                  Bit10:  Write-only (cat,w)
//                  Bit11:  Read-only (asg,r or cat,r)
//              T2: Current assign count
//              T3: Absolute F-Cycle number
//      022:    TDATE$ current assignment started, or last assignment ended
//      023:    TDATE$ time of catalog
//      024:    H1: Initial reserve in granules for this assignment
//      025:    H1: Max Granules
//      026:    H1: Highest Granule Assigned
//      027:    H1: Highest track written
//      030:    Reserved
//      031:    Reserved
//      032:    Reserved
//      033:    Reserved
//
//  Tape File Main Item Sector 0:
//      000:    Bit0:   U clear if there is a reel table, else set
//              Bit1:   always set
//              Bit2:   always clear
//              Bit3:   always clear
//              Bit4-35:Link to first reel table if U==0
//      001:    2-word fieldata qualifier
//      003:    2-word fieldata filename
//      005:    2-word fieldata project ID
//      007:    2-word fieldata account number
//      011:    Reserved
//      012:    Reserved
//      013:    S1: Disable flags
//                  Bit0:   Disabled
//                  Bit1:   Directory Error
//                  Bit2:   File written to before system stop
//                  Bit3:   Inaccessible backup
//              Bit6-35:    Link to Lead Item
//      014:    T1: Descriptor Flags
//                  Bit0:   Reserved
//                  Bit1:   Backed up
//                  Bit2:   Reserved
//                  Bit3:   Reserved
//                  Bit4:   Reserved
//                  Bit5:   To be cataloged
//                  Bit6:   Tape File
//                  Bit7:   Reserved
//                  Bit8:   Reserved
//                  Bit9:   To be made write-only
//                  Bit10:  To be made read-only
//                  Bit11:  To be dropped when assign count becomes zero
//              S3: Reserved
//              H2: Reserved
//      015:    S1: Reserved
//              Bit6-35:    Link to Main Item sector 1
//      016:    Assign mnemonic
//      017:    H1: Reserved
//              H2: Cumulative assign count
//      020:    Reserved
//      021:    S1: Shared file index (always zero)
//              S2: Inhibit flags
//                  Bit6:   Guarded file (option G)
//                  Bit7:   Unload inhibit (option V)
//                  Bit8:   Private file (option P not present on cat or asg)
//                  Bit9:   Exclusive use (asg,X)
//                  Bit10:  Write-only (cat,w)
//                  Bit11:  Read-only (asg,r or cat,r)
//              T2: Current assign count
//              T3: Absolute F-Cycle number
//      022:    TDATE$ current assignment started, or last assignment ended
//      023:    TDATE$ time of catalog
//      024:    S1: Density
//                  01 = 800 BPI or 38000BPI (U40 or USR5073 HIC)
//                  02 = 1600 BPI
//                  03 = 6250 BPI
//                  04 = 5090 BPmm (HIS98)
//                  05 = 38000BPI (U47)
//                  06 = 76000BPI (U47M, U5136, U5236 36-track drive)
//                  07 = 85937BPI (U7000)
//              S2: Format
//                  01 = Data converter (not used)
//                  02 = Quarter word
//                  04 = six bit packed (not used)
//                  010 = eight bit packed
//                  020 = even parity
//                  040 = 9 track tape
//              S3: Features (unsupported)
//              H2: Number of reels cataloged
//      025:    T1: Data compression (unsupported)
//              S3: MTAPOP
//                  040 = cat,J option
//                  020 = QIC tape
//                  010 = HIC tape
//                  004 = Buffered write off
//                  002 = DLT cartridge tape
//                  001 = Half-inch serpentine (HIS)
//              S4: Reserved
//              S5: Reserved
//              S6: Noise constant
//      026:    2-words processor tape translator mnemonics (we don't do this)
//      030:    2-words CTL pool (unused - we have something different we do)
//      032:    First reel number
//      033:    Second reel number
//
//  Main Item Sector 1
//      000:    Bit0:   U: clear if there is a main item sector 2, set otherwise
//              Bit1:   clear
//              Bit2:   clear
//              Bit3:   clear
//              Bit6-35:link to main item sector 2 if U is clear
//      001:    2-word fieldata qualifier
//      003:    2-word fieldata filename
//      005:    "*NO.1*" in fieldata
//      006:    link to main item sector 0
//      007:    T1: Number of backup words
//              T3: Absolute file cycle
//      010:    TDATE$ of backup
//      011:    S1: Max backup levels (we support only 1 level)
//              S2: FAS bits
//                  Bit6:   File was unloaded at time of backup
//                  Bit7:   Bit file - number of blocks value expressed as positions; if clear, expressed as tracks
//              S3: Current backup levels
//              H2: number of 1800-word blocks (or number / 64; see S2)
//      012:    Starting file position of first backup reel
//      013:    Reel number of first reel of backup
//      014:    Reel number of second reel of backup (or start of multiple backup information)
//      015:    Reserved
//      016:    Reserved
//      017:    S2: TIP usage
//                  0 = not used by TIP
//                  1 = Leg 1 of TIP duplex file
//                  2 = Leg 2 of TIP duplex file
//                  3 = TIP simplex file
//      020-033:Reserved
//
//  DAD sector:
//      Describes up to 8 allocations or voids (DAD entries) for a particular file cycle.
//      Linked from main item sector 0 for all mass-storage files.
//      000:    DSLINK to next DAD sector
//      001:    DSLINK to prev DAD sector
//      002:    File-relative address of 1st word accounted for by the first DAD entry in this sector
//      003:    File-relative address, plus 1, of last word accounted for by the last DAD entry in this sector
//      004:    1st DAD entry (3 words)
//              004:  DRWA where the indicated region of storage is located (on disk)
//                    If H2 of 3rd word of entry is 0400000, this is zero, since the region is unallocated.
//              005:  Number of contiguous words for this region of storage.
//                    Since space is allocated on track granularity, this must be divisible by 1792
//              006:    H1: DAD flags
//                          bit7:   No file caching
//                          bit15:  This is the last DAD entry in the containing table (sector)
//                      H2: Device index - The LDAT for allocated entries; 0400000 for unallocated.
//      007:    2nd DAD entry (if existing)
//          ...
//      031:    8th DAD entry (if existing)
//
//  Reel Table Sector:
//      When a cataloged tape file has more than 2 reel numbers, entries for the third through the nth
//      reel number are stored in successive linked reel table sectors.
//      000:    Bit0:   U: clear if there is a next sector, set otherwise
//              Bit1:   clear
//              Bit2:   clear
//              Bit3:   clear
//              Bit6-35:link to next reel table sector if U is clear, else zero
//      001:    DSLINK to previous reel table sector for 2nd and subsequent tables;
//                  links to main item sector 1 for the first reel table sector.
//      002:    Reel number 3 ( or 28, 53, etc )
//      003:    Reel number 4 ( or 29, 54, etc )
//      ...
//      032:    Reel number 27 ( etc )
//      033:    Reserved

//  Messages we might need
//  device BAD DIRECTORY ON PACK pack - id
//      (Exec) A bad directory track was found when checking the label of a pack.No
//      response is required.However, if the pack needs to be prepped, contact your
//      systems analyst.
//  BAD DIR TRACK / BAD DAS LINK - disk address
//      device PARTIALLY RECOVERED, CONTINUE - Y N
//      (Exec) A mass storage unit is partially unrecoverable and some, but not all, files
//      or parts of files residing on the unit are lost.
//  DEVICE device, PACK CONTAINS BAD SECTOR1
//      (Exec) During a recovery boot, the pack cannot be recovered due to bad sectors.
//  DEVICE device, PACK CONTAINS BAD VOL1
//      (Exec) During a recovery boot, an incorrect volume label was detected and the
//      pack cannot be recovered
//  DEVICE / PACKID nnnnnn NOT RECOVERED AS FIXED
//      (Exec) The device or pack - id nnnnnn that was a fixed mass storage device in the
//      previous boot session was not recovered during this boot.
//  DEVICE devnam PACKS LARGER THAN 67GB ARE NOT SUPPORTED.
//  Discard data for packs or reboot with packs up - DISCARD or REBOOT ?
//      (Exec)At the end of label checking during file recovery, the pack - id / LDAT table
//      contained entries for disks that were not recovered.These unrecovered entries
//      are for disks that are not up, so neither cataloged file recovery nor pack registration
//      can recover the files.Data can only be discarded unless the operator chooses to
//      reboot the system and bring up the disks.Specify DISCARD or REBOOT.
//      DISCARD results in the data in the XPC being purged and registration of the
//      pack results in files being data disabled.
//      REBOOT reboots the system after an 055 stop.This is the only mechanism
//      to avoid discarding data and data disabling files.
//      Allow an initializing, reemploying, or recovering host to change the
//      directory association of a configured shared device preceding local
//      file initialization or local file recovery.
//  DISC REGISTRATION CONFLICT OR OPERATOR TERMINATION
//      (Exec) The disk registration process was aborted.
//  DO YOU WISH TO CHANGE THE PACKID ON device Y, N
//      (Exec) Specify whether a new pack - id is desired on the migrating disk.
//      Y displays the following message :
//          ENTER NEW PACKID FOR pack - id ON device
//          You must respond with a new pack - id in the range of 1 to 6 alphanumeric
//          characters.If you fail to respond with a proper pack - id, a new pack - id is
//          resolicited
//      N does not change the disk pack - id.
//  dddddd device DUPLICATE PACK pack - id - IGNORED
//      (Exec) A premounted pack has a duplicate pack - id of a fixed or currently used
//      removable pack.The request to mount the pack will be ignored.
//      dddddd is the pack directory(LOCAL or SHARED).
//  pack - id number FILES REMOVED
//      (Exec) The specified number of files were removed for the specified device.
//  dir - id FIXED PACK MOUNTED ON device IGNORED
//      (Exec) This message confirms that the N(no) response was given to the message :
//      dir - id pack - id TO BECOME FIXED YN ?
//      The addition of the fixed pack was rejected.
//  INITIALIZATION FOR PACK disk - name HAS BEGUN
//      (Exec) During system initialization, the Exec has detected a disk that has no VOL1
//      label.The Exec transparently writes the VOL1 label and directory tracks to the pack.
//      This process takes about 8 to 10 minutes for the 8451 disks, but is instantaneous
//      for all other packs.The pack is initialized with a fixed prep type, and a disk - resident
//      system(DRS) area is allocated.
//  INITIALIZATION FOR PACK pack - id HAS COMPLETED
//      (Exec) The initialization of pack pack - id has finished.Directory tracks and the VOL1
//      label are written.
//  device - INVALID DEVICE NO.n
//  device UNRECOVERABLE, ANS D, I, or X
//      (Exec) This message is displayed for one of two reasons :
//      * An entire mass storage unit is unrecoverable; all files or parts of files that reside
//          on the unit are lost.
//      * During a recovery boot, the pack - id / LDAT table entry pack - id for the LDAT index
//          did not match the pack - id in Sector 1 for a fixed device.
//      D brings down the unit and lets the recovery process continue with other units.
//      I initializes the unit, purges all files on the unit, and lets the recovery process
//          continue with other units.
//      X causes an immediate system stop before purging any files.Call the site administrator.
//  IS PACK - ID pack - id ON device TO BECOME FIXED ? YN
//      (Exec) A fixed pack has been mounted on a removable drive.
//  IS PACK pack - id ON device YN
//      (Exec) An attention interrupt is received from a reserved disk unit that has a freeformat
//      pack mounted and a user assigned to the unit.
//  LOAD pack - id device run - id id1 / id2 / . . . / idn
//      (Exec) Mount the indicated pack on the indicated disk unit.
//  LOCAL pack - id DUPLICATE PACK pack - id - IGNORED
//      (Exec) You attempted to access a duplicate pack - id.However, since the local
//      pack - id is currently open, the duplicate request for pack - id is ignored.
//  LOCAL device WRONG PACK, LOAD pack - id AE ?
//      (Exec)Either of the following situations occurred :
//      * Situation 1
//          A conflict exists between the pack - id requested and the information on the disk
//          pack label record.You have two options :
//          - Mount the correct pack and answer A.
//          - Terminate the pack access by answering E.
//      * Situation 2
//          An I / O error has occurred while the Exec was attempting to read the information
//          on a pack's label record.You have two options :
//          - Eliminate the cause of the I / O error(if possible) and answer A.
//          - If the pack is assigned or the I / O was requested by the Exec, an E response
//          is fatal(another message will confirm this before stopping the system).
//          device is a drive for physically removable disks(for example, 8433).
//  pack - id ON device FIXED nn WORDS / RECORD dir - id
//      (Exec) You tried to use an FA, IN, or UP keyin for a device prepped as fixed.
//  pack - id ON device REMOV nnn WORDS / RECORD dir - id
//      (Exec) You tried to use an FA, IN, or UP keyin for a device prepped as removable.
//      The character strings nnn refer to the prep factor of the pack in words per record,
//      and dir - id is the directory - id STD or SHARED.
//  dir - id PACK pack - id CANNOT BE REGISTERED, ANS A OR D
//      (Exec) You tried to register a removable pack, which is an unrecoverable error.
//      A continues the run without registering the pack
//      D brings down the system.
//  dir - id PACK pack - id DIRECTORY TRACK LOST, FILES LOST
//      (Exec) A directory track was lost while you registered a removable pack.Pack
//      removal has finished, but the pack still contained some cataloged files when you
//      performed the RP keyin.
//  dir - id PACK pack - id number FILES REMOVED
//      (Exec) The files on the specified pack - id were removed from the master file
//      directory.You get this message after an RP keyin.
//  PACKID IN SECTOR1 AND VOL1 DO NOT MATCH
//      (Exec)This message(from DLABEL) is displayed when the pack - id from VOL1 of
//      the pack that is stored in LDUMID(in LDUST) does not match the pack - id held in
//      Sector 1 on the pack
//  dir - id PACK pack - id IS NOW REGISTERED
//      (Exec) The removable pack registration was completed successfully.
//  dir - id PACK pack - id LOST FILE
//      (Exec) A file track was lost while you registered a removable pack.
//  dir - id PACK pack - id NO FILES CATALOGED
//      (Exec) You performed an RP keyin for a pack without any cataloged files.
//  dir - id PACK pack - id NOT CURRENTLY REGISTERED
//      (Exec) You tried to do an RP keyin on a previously unregistered pack.
//  PACKID IN SECTOR1 AND VOL1 DO NOT MATCH
//      (Exec) This message(from DLABEL) is displayed when the pack - id from VOL1 of
//      the pack that is stored in LDUMID(in LDUST) does not match the pack - id held in
//      Sector 1 on the pack.
//  PLT entry for pack - id is not valid.
//  No entry for SECTOR1 LDAT = n(LDUST LDAT is n)
//  Entry has packid : pack - id; SECTOR1 has packid : pack - id
//  Status n received on PLT request for LDAT n
//  DN device or stop the system, ans D or X
//      (Exec) This message is displayed during a recovery boot if the PLT entry pack id
//      for the LDAT index does not match the pack - id in Sector 1 for a fixed device.It is
//      also displayed if no PLT entry is found for an UP fixed device which could occur on
//      a tape recovery if a device is found UP that was not UP during the previous session.
//  REGISTER CONFLICT: FILE qual*filename(cycle)
//      (Exec)A removable disk file with the same name as the one you are trying to
//      register already exists in the master file directory.
//  REGISTER TO CONTINUE ? ANS Y, N, OR D
//      (Exec) There have been six conflicts when trying to register a removable pack.
//      Y continues pack registration with some files lost.
//      N asks you to confirm your intent to abort the registration. (No answer is
//      required.)
//      elicits another message asking whether pack registration should
//      continue.
//      D asks you to confirm your decision to bring down the system, as follows :
//      YES stops the system.
//  Removable pack overflow - Nofity the Site Administrator
//  File dir#qual*filename(cycle) allocation request for
//  run - id runid was rejected.File spans <n> pack(s).
//  Pack - ids are : <pack1>, <pack2>, <pack3>, <pack4>, <pack5>, . . .
//      (Exec)This message, which is dynamically configurable(pack_overflow_message),
//      also has a message display threshold(MAX_REM_PACK_MSG_A_MINUTE / 10 PER
//      MINUTE) to prevent system console flooding.This message is displayed when the
//      Exec is unable to satisfy a mass storage file allocation request on a removable disk
//      because of the availability(refer to FS, AVAIL keyin for more information regarding
//      disk availability).This message will display up to 5 pack - ids.
//      Pack - id is : pack 1.
//      Pack - ids are : pack1 <, pack2><, pack 3><, pack 4><, pack5><, . . .>
//  SECTOR-1 HAS AN INVALID VERSION NUMBER
//      (Exec)The version number in sector - 1 is not valid.Legal version numbers are
//      0 and 1. You must prep the device with a Legal version number before it can be
//      used.
//  SECTOR - 1 HAS AN INVALID DAS OFFSET
//      (Exec) The number of sectors reserved for sector - 0, sector - 1, and the master bit
//      tables(MBT) is not valid.Legal values are 0 or an even multiple of 576 (octal 1100).
//      You must prep the device before it can be used.
//  SERVICE pack - id device run - id id1 / id / . . . / idn
//      (Exec) This message is issued every 2 minutes to notify that a removable pack
//      LOAD request is outstanding
//  device WRONG PACK, LOAD pack-id AE?



#include    "execlib.h"



#ifdef  _DEBUG
#define     OVERWRITTEN_SEARCH_ITEM     (0111111111111ll)
#define     OVERWRITTEN_LEAD_ITEM       (0222222222222ll)
#define     OVERWRITTEN_MAIN_ITEM       (0333333333333ll)
#define     OVERWRITTEN_DAD_TABLE       (0444444444444ll)
#define     OVERWRITTEN_REEL_TABLE      (0555555555555ll)
#endif



//  statics

const std::string   MFDFQualifier               = "SYS$";
const std::string   MFDFFileName                = "MFDF$$";



//  TODO:DEBUG - this needs removed at some point, once we're happy that things are going nicely
#define DEBUG_INSERT(sector)            \
{                                       \
    assert( sector != 0 );              \
    m_UpdatedSectors.insert(sector);    \
}



//  private methods

//  allocateDirectorySector()
//
//  Locates an unused directory sector - if none are available, a new directory track
//  is created, and the first unused sector from that track is chosen --
//  (that track may or may not be a DAS track).
MFDManager::Result
MFDManager::allocateDirectorySector
(
    Activity* const     pActivity,
    const LDATINDEX     preferredLDATIndex,
    DSADDR* const       pDSAddress
)
{
    Result result;

    //TODO:DEBUG
    std::stringstream strm;
    strm << "MFDManager::allocateDirectorySector() preferredLDAT=0" << std::oct << preferredLDATIndex;
    SystemLog::write(strm.str());
    //END DEBUG

    //  Check DAS's on the various packs to find one which has a free directory sector.
    //  Start with the preferred pack, then move on as necessary.  At this point, we
    //  do not consider allocating a new track.

    //  Set up loop, iterate once for each fixed pack, calling subfunction to do the nasty work.
    //  If the preferred pack is not found or not in the fixed pool, start at the top.
    ITPACKINFOMAP itpi = m_PackInfo.find( preferredLDATIndex );
    if ( (itpi == m_PackInfo.end()) || !itpi->second->m_InFixedPool )
    {
        itpi = m_PackInfo.begin();
        while ( !itpi->second->m_InFixedPool )
        {
            ++itpi;
            if ( itpi == m_PackInfo.end() )
            {
                //  We only get here if there aren't any packs in the fixed pool...
                //  if that's the case, we shouldn't have ever got to this point to begin with.
                SystemLog::write("MFDManager::allocateDirectorySector() no packs in fixed pool");
                result.m_Status = MFDST_INTERNAL_ERROR;
                return result;
            }
        }
    }

    //  We're at a good starting point - note that starting point.
    //  Then try to allocate a directory sector on this pack.
    //  If it fails, keep trying subsequent fixed packs until it works, or we run out of packs.
    ITPACKINFOMAP itpiStart = itpi;
    while ( true )
    {
        //  Check to ensure the pack is in the fixed pool - retries may give us a removable pack here.
        if ( itpi->second->m_InFixedPool )
        {
            result = allocateDirectorySectorOnPack( pActivity, itpi->first, pDSAddress );
            if ( result.m_Status == MFDST_SUCCESSFUL )
                return result;
        }

        //  Must not be an available directory sector on the pack (or this one is removable) - try the next one.
        ++itpi;
        if ( itpi == m_PackInfo.end() )
            itpi = m_PackInfo.begin();

        //  Don't keep trying forever, though...
        if ( itpi == itpiStart )
            break;
    }

    //  Allocate a new directory track and try again.
    result = allocateDirectoryTrack( pActivity, preferredLDATIndex );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    //  As mentioned, we are guaranteed to have a sector now, so no worries about infinite recursion...
    return allocateDirectorySector( pActivity, preferredLDATIndex, pDSAddress );
}


//  allocateDirectorySectorOnDAS()
//
//  Checks the DAS for any tracks which have an available directory sector.
//  If one is found, it is allocated for the caller, and the DSADDR for the sector is passed back.
MFDManager::Result
MFDManager::allocateDirectorySectorOnDAS
(
    Activity* const     pActivity,
    const DSADDR        dasAddr,
    DSADDR* const       pDSAddress
)
{
    static UINT64 entryMask = 0777777777760;
    Result result;

    //TODO:DEBUG
    std::stringstream strm;
    strm << "MFDManager::allocateDirectorySectorOnDAS() dasAddr=0" << std::oct << dasAddr;
    SystemLog::write( strm.str() );
    //ENDDEBUG

    //  stage the DAS
    Word36* pDAS = 0;
    if ( !stageDirectorySector( dasAddr, false, &pDAS, &result ) )
        return result;

    //  Iterate over the 9 entries in DAS.
    Word36* pEntry = pDAS;
    for ( INDEX ex = 0; ex < 9; ++ex )
    {
        //  Is this entry in use?
        //  First entry is the DAS track, so it's automatically in use
        //  Subsequent entries are in use if their DRWA link is non-negative.
        if ( (ex == 0) || (!pEntry[0].isNegative()) )
        {
            //  Iterate over the words in the entry (2 of them)
            for ( INDEX wx = 0; wx < 2; ++wx )
            {
                //  If any of the bits in the entryMask area (most significant 32 bits)
                //  are clear, then we have a sector we can use.
                if ( (pEntry[wx + 1].getW() & entryMask) != entryMask )
                {
                    //  Calculate DSADDR by starting with DASAddr, adding entry and word offsets,
                    //  and finally shifting the entry itself, incrementing until we find the 1st clear bit.
                    UINT64 bitMask = 0400000000000;
                    *pDSAddress = dasAddr + static_cast<DSADDR>((64 * ex) + (32 * wx));
                    UINT64 entry = pEntry[wx + 1].getW();
                    while ( (entry & bitMask) != 0 )
                    {
                        bitMask >>= 1;
                        ++(*pDSAddress);
                    }

                    //  Update the entry to indicate the sector to-be in-use, and mark the DAS as updated.
                    //  This will not move the sector, so pDAS and pEntry will still be valid.
                    pEntry[wx + 1].logicalOr( bitMask );
                    DEBUG_INSERT( dasAddr );
                    //TODO:DEBUG
                    strm.str("");
                    strm << "    Returning with pDSAddress->0" << std::oct << *pDSAddress;
                    SystemLog::write(strm.str());
                    //ENDDEBUG
                    return result;
                }
            }
        }

        pEntry += 3;
    }

    result.m_Status = MFDST_OUT_OF_SPACE;
    return result;
}


//  allocateDirectorySectorOnPack()
//
//  Locates an unused directory sector on the specified pack, if any are available.
//  Returns m_Status == MFDST_SUCCESSFUL, MFDST_INTERNAL_ERROR, or MFDST_OUT_OF_SPACE.
//  Subordinate functions may return other statuses.
//  Does not stop the exec for any reason.
MFDManager::Result
MFDManager::allocateDirectorySectorOnPack
(
    Activity* const         pActivity,
    const LDATINDEX         ldatIndex,
    DSADDR* const           pDSAddress
)
{
    Result result;
    //TODO:DEBUG
    std::stringstream strm;
    strm << "MFDManager::allocateDirectorySectorOnPack() ldat=0" << std::oct << ldatIndex;
    SystemLog::write( strm.str() );
    //ENDDEBUG

    //  Grab sectors 0 and 1
    DSADDR sector0Addr = (ldatIndex << 18);
    Word36* pSector0 = 0;
    if ( !stageDirectorySector( sector0Addr, false, &pSector0, &result ) )
        return result;

    DSADDR sector1Addr = sector0Addr + 1;
    Word36* pSector1 = 0;
    if ( !stageDirectorySector( sector1Addr, false, &pSector1, &result ) )
        return result;

    //  Branch on DAS offset field of sector 1 - if zero, sector 0 is the first DAS.
    //  Otherwise, sector 0 contains the DRWA of the first DAS, and its DSADDR can be
    //  deduced from the number of initial directory tracks.
    DSADDR dasAddr = 0;
    Word36* pDAS = 0;
    COUNT32 dasOffset = pSector1[020].getH2();
    if ( dasOffset == 0 )
    {
        //  Sector 0 is the first DAS; it's already staged.
        dasAddr = sector0Addr;
        pDAS = pSector0;
    }
    else
    {
        //  Determine sector address of first DAS, and stage it.
        dasAddr = sector0Addr + dasOffset;
        if ( !stageDirectorySector( dasAddr, false, &pDAS, &result ) )
            return result;
    }

    //  Loop over chained DAS entries.
    while ( dasAddr != 0 )
    {
        result = allocateDirectorySectorOnDAS( pActivity, dasAddr, pDSAddress );
        if ( result.m_Status == MFDST_SUCCESSFUL )
            return result;

        //  Look for DAS link - if there isn't one, we're done (zero out pDAS to quit)
        //  Otherwise, increment dasAddr and get the next DAS.
        if ( pDAS[033].isNegative() )
            dasAddr = 0;
        else
            dasAddr += (9 * 64);
    }

    //  Nothing available on this pack.
    result.m_Status = MFDST_OUT_OF_SPACE;
    return result;
}


//  allocateDirectoryTrack()
//
//  Allocates a new directory track and integrates it into the MFD
MFDManager::Result
MFDManager::allocateDirectoryTrack
(
    Activity* const     pActivity,
    const LDATINDEX     preferredLDATIndex
)
{
    Result result;

    //TODO:DEBUG
    std::stringstream strm;
    strm << "MFDManager::allocateDirectoryTrack() called, with preferred LDAT = 0" << std::oct << preferredLDATIndex;
    SystemLog::write( strm.str() );
    //ENDDEBUG

    //  Select a fixed pack for allocation.  We can use only packs which are UP,
    //  which have at least one track unallocated, and which have < 07777 directory tracks.
    PACKINFOLIST packList;
    PackInfo* pPreferredPack = 0;
    for ( CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        PackInfo* pPackInfo = itpi->second;
        if ( pPackInfo->m_InFixedPool )
        {
            const DeviceManager::DeviceEntry* pEntry = m_pDeviceManager->getDeviceEntry( pPackInfo->m_DeviceId );
            if ( pEntry == 0 )
            {
                std::stringstream strm;
                strm << "MFDManager::allocateDirectoryTrack() no device returned for device id " << pPackInfo->m_DeviceId;
                SystemLog::write( strm.str() );
                result.m_Status = MFDST_INTERNAL_ERROR;
                return result;
            }

            //  Add packs randomly to front or back, except for preferred pack which we add to front, later
            if ( pEntry->m_Status == DeviceManager::NDST_UP )
            {
                if ( pPackInfo->m_LDATIndex == preferredLDATIndex )
                    pPreferredPack = pPackInfo;
                else if ( rand() > (RAND_MAX >> 1) )
                    packList.push_front( pPackInfo );
                else
                    packList.push_back( pPackInfo );
            }
        }
    }

    if ( pPreferredPack )
        packList.push_front( pPreferredPack );

    //  Iterate over the pack list, to find the first one which has at least one available track,
    //  and less than 07777 directory sectors.
    PackInfo* pSelectedPack = 0;
    for ( CITPACKINFOLIST itpi = packList.begin(); itpi != packList.end(); ++itpi )
    {
        //  Call up sector 1 for this pack to find out how many available tracks there are
        DSADDR sector1Addr = ((*itpi)->m_LDATIndex << 18) | 01;
        Word36* pSector1 = 0;
        if ( !stageDirectorySector( sector1Addr, false, &pSector1, &result ) )
            return result;
        TRACK_COUNT availableTracks = pSector1[3].getW() == 0;
        if ( availableTracks == 0 )
            continue;

        //  If DSADDR {ldat}|7777|00 exists, then we cannot use this
        DSADDR lastAddr = ((*itpi)->m_LDATIndex << 18) | 0777700;
        CITDIRECTORYCACHE itTest = m_DirectoryCache.find( lastAddr );
        if ( itTest == m_DirectoryCache.end() )
            continue;

        //  We can use this one!
        pSelectedPack = *itpi;
    }

    //  If there are no suitable packs, return MFDST_OUT_OF_SPACE
    if ( pSelectedPack == 0 )
    {
        SystemLog::write( "MFDManager::allocateDirectoryTrack() Out of space" );
        result.m_Status = MFDST_OUT_OF_SPACE;
        return result;
    }

    //  Allocate a track and update the SMBT
    TRACK_ID deviceTrackId = 0;
    pSelectedPack->m_DiskAllocationTable.findUnallocatedRegion( 1, &deviceTrackId );
    pSelectedPack->m_DiskAllocationTable.allocate( deviceTrackId, 1 );
    result = setSMBTAllocated( pActivity, pSelectedPack->m_LDATIndex, deviceTrackId, 1, true, false );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    //  Walk the DASs to determine the DSADDR for the first new sector.
    //  If no DAS has an empty entry, then this new track will be a DAS track.
    //  First, stage sectors 0 and 1 (we're going to update sector 1 later)
    DSADDR sector0Addr = (pSelectedPack->m_LDATIndex << 18);
    Word36* pSector0 = 0;
    if ( !stageDirectorySector( sector0Addr, false, &pSector0, &result ) )
        return result;

    DSADDR sector1Addr = sector0Addr + 1;
    Word36* pSector1 = 0;
    if ( !stageDirectorySector( sector1Addr, true, &pSector1, &result ) )
        return result;

    //  Find DSADDR of first DAS sector - if DAS offset in sector 1 is zero, then the first DAS is sector 0.
    //  Otherwise, the DAS offset indicates the DSADDR.
    DSADDR dasAddr = sector0Addr;
    SECTOR_COUNT dasOffset = pSector1[020].getH2();
    if ( dasOffset > 0 )
        dasAddr += static_cast<DSADDR>( dasOffset );

    //  Iterate over the DAS chain
    Word36* pDAS = 0;       //  Pointer to DAS with empty entry, or to last DAS in chain
    INDEX32 ex = 0;         //  Index of empty entry if found
    bool found = false;     //  true if we found a DAS with an empty entry, else false
    while ( !found )
    {
        if ( !stageDirectorySector( dasAddr, false, &pDAS, &result ) )
            return result;

        //  Iterate over the 2nd through 9th entries to find an unused entry.
        //  (1st entry is always used, referring to the DAS track itself)
        for ( ex = 1; ex < 9; ++ex )
        {
            Word36* pEntry = pDAS + (3 * ex);
            if ( pEntry->isNegative() )
            {
                //  This entry is not used - we've found what we were looking for.
                found = true;
                break;
            }
        }

        if ( pDAS[033].isNegative() )
        {
            //  No more DAS tracks - stop here.
            break;
        }

        //  Follow DAS chain
        dasAddr = getLinkAddress( pDAS[033] );
    }

    //  Determine DSADDR for new track
    DSADDR newTrackDSAddr = found ? dasAddr + (ex << 6) : dasAddr + (9 << 6);
    bool newTrackIsDAS = !found;

    //  Instantiate the track in cache, and zero out all the sectors, marking them updated.
    for ( INDEX32 sx = 0; sx < SECTORS_PER_TRACK; ++sx )
    {
        Word36* pSector = 0;
        if ( stageDirectorySector( newTrackDSAddr + sx, true, &pSector, &result ) )
            return result;
        for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
            pSector[wx].clear();
    }

    //  If this track is a DAS track, create the DAS in sector 0 and link it to the end of the previous last DAS.
    if ( newTrackIsDAS )
    {
        Word36* pNewDAS = 0;
        if ( stageDirectorySector( newTrackDSAddr, true, &pNewDAS, &result ) )
            return result;
        pNewDAS[0].setH1( pSelectedPack->m_LDATIndex );     //  LDAT index in word 0 H1
        pNewDAS[1].setW( 0400000000000ll );                 //  bit mask indicating first sector is used
        for ( INDEX ey = 1; ey < 9; ++ey )
            pNewDAS[3 * ey] = ( 0400000000000ll );          //  links 2 through 9 are empty/available

        pDAS[033].setW( deviceTrackId * WORDS_PER_TRACK );
    }

    //  Otherwise, just link this new track to the appropriate existing empty DAS entry.
    else
    {
        pDAS[3 * ex].setW( deviceTrackId * WORDS_PER_TRACK );
        pDAS[(3 * ex) + 1].clear();
        pDAS[(3 * ex) + 2].clear();
        DEBUG_INSERT( dasAddr );
    }

    //  Finally, update sector 1 for tracks available
    pSector1[3].setW( pSector1[3].getW() - 1 );

    return result;
}


//  allocateFixedTracks()
//
//  Attempts to allocate contiguous space for the caller, by polling the packs in the given list to determine best fit.
//  First, we go through the packs to see if there is any space of exactly the requested size.
//  If so, we allocate it and we're done.  Otherwise, we go through again, looking for space greater than requested,
//  so we can still allocate the entire amount of space.  Failing that, we go through one more time looking for the
//  largest block available.
//
//  Parameters:
//      pActivity:              pointer to the controlling activity
//      packList:               list of packs, in order of preference, for consideration
//      tracksRequested:        number of contiguous tracks the caller would like to have
//      temporaryFile:          if true, we do not update SMBT for tracks allocated
//      pAllocatedLDATIndex:    where we store the LDATINDEX for the pack we ended up using
//      pDeviceTrackId:         device-relative track ID of the first contiguous track we allocated
//      pTracksAllocated:       actual number of contiguous tracks we allocated (may be less than requested)
MFDManager::Result
MFDManager::allocateFixedTracks
(
    Activity* const         pActivity,
    const PACKINFOLIST&     packList,
    const TRACK_COUNT       tracksRequested,
    const bool              temporaryFile,
    LDATINDEX* const        pAllocatedLDATIndex,
    TRACK_ID* const         pDeviceTrackId,
    TRACK_COUNT* const      pTracksAllocated
)
{
    Result result;
    bool done = false;

    //  Look for space of the exact size requested
    for ( CITPACKINFOLIST itpi = packList.begin(); itpi != packList.end(); ++itpi )
    {
        PackInfo* pPackInfo = *itpi;
        if ( pPackInfo->m_DiskAllocationTable.findUnallocatedRegion( tracksRequested, pDeviceTrackId ) )
        {
            pPackInfo->m_DiskAllocationTable.allocate( *pDeviceTrackId, tracksRequested );
            *pTracksAllocated = tracksRequested;
            *pAllocatedLDATIndex = pPackInfo->m_LDATIndex;
            done = true;
        }
    }

    //  Look for space greater than the requested size
    if ( !done )
    {
        for ( CITPACKINFOLIST itpi = packList.begin(); itpi != packList.end(); ++itpi )
        {
            PackInfo* pPackInfo = *itpi;
            if ( pPackInfo->m_DiskAllocationTable.findLargerUnallocatedRegion( tracksRequested, pDeviceTrackId ) )
            {
                pPackInfo->m_DiskAllocationTable.allocate( *pDeviceTrackId, tracksRequested );
                *pTracksAllocated = tracksRequested;
                *pAllocatedLDATIndex = pPackInfo->m_LDATIndex;
                done = true;
            }
        }
    }

    //  Finally, iterate over all the packs to find the largest unallocated region available.
    if ( !done )
    {
        LDATINDEX   ldatIndexLargest    = 0;
        TRACK_COUNT trackCountLargest   = 0;
        TRACK_ID    trackIdLargest      = 0;
        for ( CITPACKINFOLIST itpi = packList.begin(); itpi != packList.end(); ++itpi )
        {
            PackInfo* pPackInfo = *itpi;
            TRACK_COUNT trackCount;
            TRACK_ID trackId;
            if ( pPackInfo->m_DiskAllocationTable.findLargestUnallocatedRegion( &trackId, &trackCount ) )
            {
                if ( trackCount > trackCountLargest )
                {
                    ldatIndexLargest = pPackInfo->m_LDATIndex;
                    trackCountLargest = trackCount;
                    trackIdLargest = trackId;
                }
            }
        }

        if ( trackCountLargest > 0 )
        {
            *pAllocatedLDATIndex = ldatIndexLargest;
            *pDeviceTrackId = trackIdLargest;
            *pTracksAllocated = trackCountLargest;
            done = true;
        }
    }

    //  Are we out of space?
    if ( !done )
    {
        result.m_Status = MFDST_OUT_OF_SPACE;
        return result;
    }

    //  Update the SMBT to account for the newly-allocated space.
    result = setSMBTAllocated( pActivity, *pAllocatedLDATIndex, *pDeviceTrackId, *pTracksAllocated, true, false );

    //  Update sector 1 for tracks available
    DSADDR sector1Addr = ((*pAllocatedLDATIndex) << 18) + 1;
    Word36* pSector1 = 0;
    if ( !stageDirectorySector( sector1Addr, true, &pSector1, &result ) )
        return result;
    pSector1[3].setW( pSector1[3].getW() - *pTracksAllocated );

    //TODO:DEBUG
    std::stringstream strm;
    strm << "MFDManager::allocateFixedTracks() alloc'd 0" << std::oct << *pTracksAllocated
        << " track(s) on LDAT 0" << std::oct << *pAllocatedLDATIndex
        << " at Device TrackID 0" << std::oct << *pDeviceTrackId;
    SystemLog::write( strm.str() );
    //ENDDEBUG

    return result;
}


//  bringFixedPackOnline()
//
//  Initializes a single fixed pack, bringing it into the fixed pool.
//  Used for one-at-a-time initializations.  Bulk init during boot is done elsewhere.
//  Call under lock().  Caller must commit MFD changes.
//
//  Parameters:
//      pActivity:          pointer to requesting Activity object
//      pNodeEntry:         pointer to DeviceManager's NodeEntry object for the device
//      pPackInfo:          pointer to newly-acquired PackInfo object for the device - we own it now.
MFDManager::Result
    MFDManager::bringFixedPackOnline
    (
    Activity* const                         pActivity,
    const DeviceManager::NodeEntry* const   pNodeEntry,
    PackInfo* const                         pPackInfo
    )
{
    Result result;

    //  Check pack name to avoid name conflicts.
    //  While we're here, remove any existing entries for the same pack.
    //  But wait!  Name conflict, existing entry, isn't that the same thing?
    //  Well, kind of.  So what to do?  If we find a match on pack name, check whether the pack is mounted.
    //  If so, we have a conflict.  If not, we have a stale entry to be deleted.
    ITPACKINFOMAP itpi = m_PackInfo.begin();
    while ( itpi != m_PackInfo.end() )
    {
        if ( pPackInfo->m_PackName.compareNoCase( itpi->second->m_PackName ) == 0 )
        {
            //  Pack name match - what to do?
            if ( itpi->second->m_IsMounted )
            {
                //  Conflict!
                std::string consMsg = "DEVICE ";
                consMsg += m_pDeviceManager->getNodeName( pPackInfo->m_DeviceId ) + " PACK-ID CONFLICT";
                m_pConsoleManager->postReadOnlyMessage( consMsg, 0 );
                m_pDeviceManager->setNodeDownInternally( pActivity, pNodeEntry->m_NodeId );
                result.m_Status = MFDST_PACK_NAME_CONFLICT;
                return result;
            }
            else
            {
                //  Stale!
                itpi = m_PackInfo.erase( itpi );
            }
        }
        else
        {
            //  No match - move on
            ++itpi;
        }
    }

    //  Assign an LDAT and put the PackInfo object into m_PackInfo table.
    LDATINDEX newLdat = chooseFixedLDATIndex();
    m_PackInfo[newLdat] = pPackInfo;
    itpi = m_PackInfo.find( newLdat );

    //  Initialize the fixed pack.
    result = initializeFixedPack( pActivity, itpi );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        //  Set the device DN and remove and delete the PackInfo object
        m_pDeviceManager->setNodeDownInternally( pActivity, pNodeEntry->m_NodeId );
        PackInfo* pPackInfo = itpi->second;
        m_PackInfo.erase( itpi );
        delete pPackInfo;
        return result;
    }

    //  Load Disk Allocation Table for the fixed pack based on the SMBTs.
    result = loadFixedPackAllocationTable( pActivity, itpi );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        //  Set the device DN and remove and delete the PackInfo object
        m_pDeviceManager->setNodeDownInternally( pActivity, pNodeEntry->m_NodeId );
        PackInfo* pPackInfo = itpi->second;
        m_PackInfo.erase( itpi );
        delete pPackInfo;
        return result;
    }

    //  Notify operator that the pack is ready to go.
    std::stringstream strm;
    strm << pPackInfo->m_PackName
        << " ON "
        << pNodeEntry->m_pNode->getName()
        << " FIXED "
        << pPackInfo->m_PrepFactor
        << " WORDS/RECORD";
    m_pConsoleManager->postReadOnlyMessage( strm.str(), 0 );

    return result;
}


//  chooseFixedLDATIndex()
//
//  Pseudo-randomly selects an LDATINDEX from the fixed pool
LDATINDEX
MFDManager::chooseFixedLDATIndex()
{
    COUNT size = m_PackInfo.size();
    while ( true )
    {
        INDEX index = rand() % size;
        CITPACKINFOMAP itpi = m_PackInfo.begin();
        while ( index > 0 )
        {
            ++itpi;
            --index;
        }

        if ( itpi->second->m_InFixedPool )
        {
            //TODO:DEBUG
            std::stringstream strm;
            strm << "MFDManager::chooseFixedLDATIndex() chose LDAT 0" << std::oct << itpi->first;
            SystemLog::write(strm.str());
            //ENDDEBUG
            return itpi->first;
        }
    }
}


//  commitMFDUpdates()
//
//  Writes all updated staged MFD sectors to disk, and empties the MFD staged cache.
MFDManager::Result
MFDManager::commitMFDUpdates
(
    Activity* const     pActivity
)
{
    Result result;

    while ( m_UpdatedSectors.size() > 0 )
    {
        //  Pull updated sector's DSADDR apart into the LDAT, trackID, and sector components
        DSADDR dsAddr = *m_UpdatedSectors.begin();
        LDATINDEX ldatIndex = (dsAddr >> 18) & 07777;
        TRACK_ID dirTrackId = (dsAddr >> 6) & 07777;
        SECTOR_COUNT sectorOffset = dsAddr & 077;

        //  Find the PackInfo for the indicated LDAT
        ITPACKINFOMAP itPackInfo = m_PackInfo.find( ldatIndex );
        if ( itPackInfo == m_PackInfo.end() )
        {
            std::stringstream strm;
            strm << "MFDManager::commitMFDUpdates() PackInfo not found for LDATIndex 0" << std::oct << ldatIndex;
            SystemLog::write( strm.str() );
            result.m_Status = MFDST_INTERNAL_ERROR;
            return result;
        }

        PackInfo* pPackInfo = itPackInfo->second;
        PREP_FACTOR prepFactor = pPackInfo->m_PrepFactor;
        DeviceManager::DEVICE_ID deviceId = pPackInfo->m_DeviceId;

        //  Find the device-relative block ID corresponding to the beginning of the directory track
        TRACK_ID ldatAndTrackId = (dsAddr >> 6) & 077777777;
        CITDIRECTORYTRACKIDMAP itBlockId = m_DirectoryTrackIdMap.find( ldatAndTrackId );
        if ( itBlockId == m_DirectoryTrackIdMap.end() )
        {
            std::stringstream strm;
            strm << "MFDManager::commitMFDUpdates() BlockId not found LDATIndex 0"
                << std::oct << ldatIndex << " TrackId 0" << std::oct << dirTrackId;
            SystemLog::write( strm.str() );
            result.m_Status = MFDST_INTERNAL_ERROR;
            return result;
        }

        BLOCK_ID trackBlockId = itBlockId->second;

        //  Use block size (prep factor) to find the DSADDR corresponding to the block which contains the updated sector.
        //  Part of this process produces a block offset which is the number of blocks beyond the block containing
        //  the beginning of this directory track, which contains the sector of interest --
        //  We do this so we don't have to write the entire directory track; only the block we care about.
        SECTOR_COUNT sectorsPerBlock = SECTORS_PER_BLOCK(prepFactor);
        BLOCK_COUNT blockOffset = sectorOffset / sectorsPerBlock;
        DSADDR startAddr = (ldatIndex << 18) | static_cast<DSADDR>(dirTrackId << 6) | static_cast<DSADDR>(blockOffset * sectorsPerBlock);
        BLOCK_ID ioBlockId = trackBlockId + blockOffset;

        //  Set up a buffer for the block, and copy all the cached sectors to that block,
        //  whether they are updated or not.  In the process, remove them from the updated container.
        Word36* pBuffer = new Word36[prepFactor];
        Word36* pDest = pBuffer;
        DSADDR sourceAddr = startAddr;
        for ( INDEX sx = 0; sx < sectorsPerBlock; ++sx )
        {
            Word36* pSector = 0;
            if ( !stageDirectorySector( sourceAddr, true, &pSector, &result ) )
                return result;

            for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
                pDest[wx] = pSector[wx];
            m_UpdatedSectors.erase( sourceAddr );

            ++sourceAddr;
            pDest += WORDS_PER_SECTOR;
        }

        //  Buffer is ready - write it
        result = directDiskIo( pActivity, deviceId, ChannelModule::Command::WRITE, ioBlockId, prepFactor, pBuffer );
        delete[] pBuffer;

        if ( result.m_Status != MFDST_SUCCESSFUL )
            return result;
    }

    return result;
}


//  deallocateDirectorySector()
//
//  Marks a directory sector as unallocated.
MFDManager::Result
MFDManager::deallocateDirectorySector
(
    Activity* const     pActivity,
    const DSADDR        dsAddress
)
{
    Result result;

    //  Calculate DSADDR of the DAS which references the indicated directory sector.
    TRACK_ID dsTrackId = dsAddress >> 6;
    DSADDR dasAddress = static_cast<DSADDR>(dsTrackId << 6);

    //  Find the DAS
    Word36* pDAS = 0;
    if ( !stageDirectorySector( dasAddress, true, &pDAS, &result ) )
        return result;

    //  Calculate the directory sectors sector and track offsets
    //  so that we can find the appropriate allocation bit in the DAS.
    COUNT dsSectorOffset = dsAddress & 077;
    COUNT dsTrackOffset = dsTrackId % 9;

    Word36* pDASEntry = pDAS + (3 * dsTrackOffset);
    INDEX entryWordIndex = 1 + (dsSectorOffset / 32);
    INDEX bitIndex = dsSectorOffset % 32;
    UINT64 bitMask = 0400000000000l >> bitIndex;

    //  Update the DAS (don't assume the bit is set, although it /should/ be).
    if ( pDASEntry[entryWordIndex].getW() & bitMask )
        pDASEntry[entryWordIndex].setW( pDASEntry[entryWordIndex].getW() ^ bitMask );

    return result;
}


//  deallocateDirectorySectors()
//
//  Calls deallocateDirectorySector() for each of the addresses in the given set of DSADDRs.
//  Returns the result of the first failure (upon which it stops), or else SUCCESS.
MFDManager::Result
MFDManager::deallocateDirectorySectors
(
    Activity* const             pActivity,
    const std::set<DSADDR>&     dsAddresses
)
{
    Result result;
    for ( DSADDR addr : dsAddresses )
    {
        result = deallocateDirectorySector( pActivity, addr );
        if ( result.m_Status != MFDST_SUCCESSFUL )
            break;
    }
    return result;
}


//  deallocateFixedTracks()
//
//  Should ONLY be used in conjunction with an update to a FileAllocationTable.
//  Releases the indicated tracks from the DiskAllocationTable for the associated pack,
//  echoes this release in the SMBT (for non-temporary files).
//
//  Caller must commit MFD updates at some point.
MFDManager::Result
MFDManager::deallocateFixedTracks
(
    Activity* const      pActivity,
    const LDATINDEX      ldatIndex,
    const TRACK_ID       trackId,
    const TRACK_COUNT    trackCount,
    const bool           temporaryFile
)
{
    Result result;

    CITPACKINFOMAP itpi = m_PackInfo.find( ldatIndex );
    if ( itpi == m_PackInfo.end() )
    {
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    PackInfo* pPackInfo = itpi->second;
    pPackInfo->m_DiskAllocationTable.release( trackId, trackCount );
    result = setSMBTAllocated( pActivity, ldatIndex, trackId, trackCount, false, false );

    return result;
}


//  directDiskIo()
//
//  Direct IO version for reading or writing a block from a particular disk device.
//  Be careful using this - it will circumvent any caching we're doing.
//  Does NOT write console message on error - caller must do that, if it is appropriate to do so.
MFDManager::Result
MFDManager::directDiskIo
(
    Activity* const                 pActivity,
    const DeviceManager::DEVICE_ID  deviceId,
    const ChannelModule::Command    command,
    const BLOCK_ID                  blockId,
    const WORD_COUNT                wordCount,
    Word36* const                   pBuffer
) const
{
    Result result;

    //  Validate some parameters
    if ( wordCount > 0777777 )
    {
        std::stringstream strm;
        strm << "MFDManager::directDiskIo() wordCount=0" << std::oct << wordCount << " is invalid";
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        directDiskIoError( deviceId, command, blockId, wordCount, 0, result );
        return result;
    }

    //  Make sure the requested device is UP or SU
    const DeviceManager::DeviceEntry* pEntry = m_pDeviceManager->getDeviceEntry( deviceId );
    if ( pEntry == 0 )
    {
        std::stringstream strm;
        strm << "MFDManager::directDiskIo() Cannot get device entry for deviceId=" << deviceId;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        directDiskIoError( deviceId, command, blockId, wordCount, 0, result );
        return result;
    }

    if ( (pEntry->m_Status != DeviceManager::NDST_UP) && (pEntry->m_Status != DeviceManager::NDST_SU) )
    {
        std::stringstream strm;
        strm << "MFDManager::directDiskIo() Device is not UP or SU deviceId=" << deviceId;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_DEVICE_NOT_ACCESSIBLE;
        directDiskIoError( deviceId, command, blockId, wordCount, 0, result );
        return result;
    }

    //  Get a path to the device - this is retryable, as the operator may have inadvertantly DN'd something.
    //  We'd like for him to be able to correct the issue, and retry.
    const DeviceManager::Path* pPath = m_pDeviceManager->getNextPath(deviceId);
    if ( pPath == 0 )
    {
        result.m_Status = MFDST_NO_PATH;
        if ( directDiskIoError( deviceId, command, blockId, wordCount, 0, result ) )
            return directDiskIo( pActivity, deviceId, command, blockId, wordCount, pBuffer );
        return result;
    }

    //  Get pointer to IOP entry for IOP which will do the IO.
    const DeviceManager::ProcessorEntry* pProcessorEntry
        = m_pDeviceManager->getProcessorEntry( pPath->m_IOPUPINumber );
    if ( !pProcessorEntry )
    {
        std::stringstream strm;
        strm << "MFDManager::directDiskIo() Cannot get pointer to IOP entry for UPI " << pPath->m_IOPUPINumber;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        directDiskIoError( deviceId, command, blockId, wordCount, 0, result );
        return result;
    }

    IOProcessor* pIOProcessor = dynamic_cast<IOProcessor*>(pProcessorEntry->m_pNode);

    //  Create channel program for the IO
    IoAccessControlWord acw(pBuffer, static_cast<COUNT>(wordCount), EXIOBAM_INCREMENT);

    ChannelModule::ChannelProgram channelProgram( pActivity );
    channelProgram.m_ProcessorUPI = pPath->m_IOPUPINumber;
    channelProgram.m_ChannelModuleAddress = pPath->m_ChannelModuleAddress;
    channelProgram.m_ControllerAddress = pPath->m_ControllerAddress;
    channelProgram.m_DeviceAddress = pPath->m_DeviceAddress;
    channelProgram.m_Address = blockId;
    channelProgram.m_AccessControlList.push_back(acw);
    channelProgram.m_Command = command;
    channelProgram.m_Format = ChannelModule::IoTranslateFormat::C;
    channelProgram.m_TransferSizeWords = wordCount;

    //  Start the IO, then wait for completion
    pIOProcessor->routeIo( &channelProgram );
    while ( channelProgram.m_ChannelStatus == ChannelModule::Status::IN_PROGRESS )
    {
        if ( pActivity->isTerminating() )
        {
            pIOProcessor->cancelIo( &channelProgram );
            result.m_Status = MFDST_TERMINATING;
            return result;
        }

        pActivity->wait( 10 );
    }

    result.m_ChannelStatus = channelProgram.m_ChannelStatus;
    result.m_DeviceStatus = channelProgram.m_DeviceStatus;
    result.m_SystemErrorCode = channelProgram.m_SystemErrorCode;
    if ( result.m_ChannelStatus != ChannelModule::Status::SUCCESSFUL )
    {
        result.m_Status = MFDST_IO_ERROR;
        if ( directDiskIoError( deviceId, command, blockId, wordCount, pPath, result ) )
            return directDiskIo( pActivity, deviceId, command, blockId, wordCount, pBuffer );
    }

    return result;
}


//  directDiskIoError()
//
//  Subordinate to directDiskIo(), we display an appropriate message to the console.
//  If channel program info is available, we display that as well.
//  Finally, if this is not an internal error, we ask the operator if a retry is in order.
//
//  Returns:
//      true for a retry
bool
MFDManager::directDiskIoError
(
    const DeviceManager::DEVICE_ID      deviceId,
    const ChannelModule::Command        channelCommand,
    const BLOCK_ID                      blockId,
    const WORD_COUNT                    wordCount,
    const DeviceManager::Path* const    pPath,
    const Result&                       result
) const
{
    const std::string& deviceName = m_pDeviceManager->getNodeName( deviceId );
    std::stringstream consStrm;

    consStrm << deviceName
        << " I/O Error on MFD Fcn=" << ChannelModule::getCommandString( channelCommand )
        << " Blk=0" << std::oct << blockId
        << " Len=0" << std::oct << wordCount;
    m_pConsoleManager->postReadOnlyMessage( consStrm.str(), 0 );

    if ( pPath )
    {
        consStrm.str( " Path=" );
        consStrm << deviceName
            << " " << m_pDeviceManager->getNodeName( pPath->m_IOPIdentifier )
            << "/" << m_pDeviceManager->getNodeName( pPath->m_ChannelModuleIdentifier )
            << "/" << m_pDeviceManager->getNodeName( pPath->m_ControllerIdentifier );
        m_pConsoleManager->postReadOnlyMessage( consStrm.str(), 0 );
    }

    consStrm.str( "" );
    consStrm << deviceName << " " + getResultString( result );
    m_pConsoleManager->postReadOnlyMessage( consStrm.str(), 0 );

    bool retry = false;
    if ( (result.m_Status != MFDST_INTERNAL_ERROR) || (result.m_Status != MFDST_DEVICE_NOT_ACCESSIBLE) )
    {
        consStrm.str( "" );
        consStrm << deviceName << " Retry IO? YN";
        VSTRING responses;
        responses.push_back( "Y" );
        responses.push_back( "N" );
        INDEX respIndex = 0;
        if ( m_pConsoleManager->postReadReplyMessage( consStrm.str(),
                                                                  responses,
                                                                  &respIndex,
                                                                  m_pExec->getRunInfo() ) )
        {
            retry = (respIndex == 0);
        }
    }

    return retry;
}


//  dropDADTables()
//
//  Drops all the DAD tables and deallocates the storage associated with the file cycle
//  indicated by the given main item sector 1.
//  Only call this for files which are NOT assigned - (otherwise, we have FAT's which lie to us).
MFDManager::Result
MFDManager::dropDADTables
(
    Activity* const         pActivity,
    const Word36* const     pMainItem1
)
{
    Result result;

    //  Iterate over the DAD tables.  Build a container of DSADDRs for DAD sectors to be deallocated,
    //  and for now deallocate all the storage indicated by the DADs.
    //  Also build a container of track counts deallocated, per LDATINDEX
    std::set<DSADDR> dsAddresses;
    std::map<LDATINDEX, TRACK_COUNT> deallocatedCounters;

    DSADDR nextDADAddr = getLinkAddress( pMainItem1[0] );
    while ( nextDADAddr )
    {
        dsAddresses.insert( nextDADAddr );
        Word36* pDADSector = 0;
#ifdef  _DEBUG
        bool updateSector = true;
#else
        bool updateSector = false;
#endif
        if ( !stageDirectorySector( nextDADAddr, updateSector, &pDADSector, &result ) )
            return result;

        //  Iterate over the entries in the DAD
        Word36* pEntry = pDADSector + 4;
        for ( INDEX ex = 0; ex < 8; ++ex )
        {
            TRACK_ID deviceTrackId = pEntry[0].getW() / 1792;
            TRACK_COUNT trackCount = pEntry[1].getW() / 1792;
            LDATINDEX ldatIndex = pEntry[2].getH2();
            bool lastEntry = (pEntry[2].getH1() & 04) != 0;
//            bool removable = (pEntry[2].getH1() & 02) != 0;//TODO:REM what to do here for removable?
            if ( ldatIndex != 0400000 )
            {
                result = setSMBTAllocated( pActivity, ldatIndex, deviceTrackId, trackCount, false, false );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                    return result;
                std::map<LDATINDEX, TRACK_COUNT>::iterator it = deallocatedCounters.find( ldatIndex );
                if ( it != deallocatedCounters.end() )
                    it->second += trackCount;
                else
                    deallocatedCounters[ldatIndex] = trackCount;
            }

            if ( lastEntry )
                break;
            pEntry += 3;
        }

        //  Debugging builds write 01...1 over the DAD sectors
#ifdef  _DEBUG
        for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
            pDADSector[wx].setW( OVERWRITTEN_DAD_TABLE );
#endif

        nextDADAddr = getLinkAddress( pDADSector[0] );
    }

    //  Deallocate the DAD table sectors
    result = deallocateDirectorySectors( pActivity, dsAddresses );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    //  Update tracks-available count in MFD sector 1 for all affected packs
    for ( std::map<LDATINDEX, TRACK_COUNT>::iterator it = deallocatedCounters.begin(); it != deallocatedCounters.end(); ++it )
    {
        DSADDR packSector1Addr = (it->first << 18) | 01;
        Word36* pSector1 = 0;
        if ( !stageDirectorySector( packSector1Addr, true, &pSector1, &result ) )
            return result;
        pSector1[3].setW( pSector1[3].getW() - it->second );
    }

    return result;
}


//  dropFileCycleInternal()
//
//  Drops a file cycle.
//  For mass storage files, we drop all the related DAD tables and deallocate the associated tracks.
//  For tape files, we drop all the related reel tables.
//  Drops all other related sectors as well.
//  Does NOT update lead item - for internal use only
MFDManager::Result
MFDManager::dropFileCycleInternal
(
    Activity* const         pActivity,
    const DSADDR            mainItem0Addr,
    const UINT8             fileType
)
{
    Result result;

    //  Read main items.
    DSADDR mainItem1Addr = 0;
    Word36* pMainItem0 = 0;
    Word36* pMainItem1 = 0;
#ifdef  _DEBUG  // see later code
    bool updateFlag = true;
#else
    bool updateFlag = false;
#endif
    if ( !stageMainItems( mainItem0Addr, updateFlag, &mainItem1Addr, &pMainItem0, &pMainItem1, &result ) )
        return result;

    //  Drop DAD tables or Reel tables, depending on the file type
    if ( fileType == 0 )
        result = dropDADTables( pActivity, pMainItem1 );
    else if ( fileType == 01 )
        result = dropReelTables( pActivity, pMainItem1 );
    //TODO:REM another else here for removable...
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    //  Build a container of DSADDRs for main item sectors 0 through n, then deallocate them all.
    std::set<DSADDR> dsAddresses;
    dsAddresses.insert( mainItem0Addr );
    dsAddresses.insert( mainItem1Addr );
    DSADDR nextAddr = getLinkAddress( pMainItem1[0] );

    //  If we're a debug build, clear out the main items
#ifdef  _DEBUG
    for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
    {
        pMainItem0[wx].setW( OVERWRITTEN_MAIN_ITEM );
        pMainItem1[wx].setW( OVERWRITTEN_MAIN_ITEM );
    }
#endif

    while ( nextAddr != 0 )
    {
        dsAddresses.insert( nextAddr );

        Word36* pNextSector = 0;
        if ( !stageDirectorySector( nextAddr, updateFlag, &pNextSector, &result ) )
            return result;

        nextAddr = getLinkAddress( pNextSector[0] );

#ifdef  _DEBUG
    for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
        pNextSector[wx].setW( OVERWRITTEN_MAIN_ITEM );
#endif
    }

    result = deallocateDirectorySectors( pActivity, dsAddresses );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    return result;
}


//  dropFileSetInternal()
//
//  Drops an entire file set.  Call only when all cycles are gone.
//  Intended to be called when code for dropping a cycle, drops the last cycle for a set.
MFDManager::Result
MFDManager::dropFileSetInternal
(
    Activity* const         pActivity,
    const DSADDR            leadItem0Addr
)
{
    Result result;

    DSADDR leadItem1Addr = 0;
    Word36* pLeadItem0 = 0;
    Word36* pLeadItem1 = 0;
    if ( !stageLeadItems( leadItem0Addr, false, &leadItem1Addr, &pLeadItem0, &pLeadItem1, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  If fileset is not empty, get mad.
    COUNT currentRange = pLeadItem0[011].getQ3();
    if ( currentRange > 0 )
    {
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    //  Remove link from search table
    result = removeLookupEntry( pActivity, &pLeadItem0[1], &pLeadItem0[3] );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  Deallocate lead item(s)
    result = deallocateDirectorySector( pActivity, leadItem0Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    if ( leadItem1Addr > 0 )
    {
        result = deallocateDirectorySector( pActivity, leadItem1Addr );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            unlock();
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    return result;
}


//  dropReelTables()
//
//  Drops all the reel tables associated with the file cycle
//  indicated by the given main item sector 1.
MFDManager::Result
MFDManager::dropReelTables
(
    Activity* const         pActivity,
    const Word36* const     pMainItem1
)
{
    Result result;

    //  Iterate over the Reel tables.  Build a container of DSADDRs to be deallocated.
    std::set<DSADDR> dsAddresses;
    DSADDR nextAddr = getLinkAddress( pMainItem1[0] );
    while ( nextAddr )
    {
        dsAddresses.insert( nextAddr );

#ifdef  _DEBUG
        bool updateFlag = true;
#else
        bool updateFlag = false;
#endif
        Word36* pReelSector = 0;
        if ( !stageDirectorySector( nextAddr, updateFlag, &pReelSector, &result ) )
            return result;

#ifdef  _DEBUG
        for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
            pReelSector[wx].setW( OVERWRITTEN_REEL_TABLE );
#endif

        nextAddr = getLinkAddress( pReelSector[0] );
    }

    //  Deallocate the reel table sectors
    result = deallocateDirectorySectors( pActivity, dsAddresses );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    return result;
}


//  dumpMFDDirectory()
//
//  Dumps the MFD data in a nice directory format.
//  Presumes that the directory is not corrupted.
//  For debugging.
void
MFDManager::dumpMFDDirectory
(
    std::ostream&       stream
)
{
    stream << "  MFD Directory:" << std::endl;

    //  Iterate over the lookup items
    for ( INDEX lx = 0; lx < m_SearchItemLookupTable.size(); ++lx )
    {
        DSADDR searchItemAddr = m_SearchItemLookupTable[lx];
        while ( searchItemAddr != 0 )
        {
            //  Read the lookup item.
            CITDIRECTORYCACHE itSearchItem = m_DirectoryCache.find( searchItemAddr );
            if ( itSearchItem == m_DirectoryCache.end() )
            {
                stream << "    Error locating search item at DSADDR 0" << std::oct << searchItemAddr << std::endl;
                continue;
            }

            const Word36* pSearchItem = itSearchItem->second;
            const Word36* pEntry = &pSearchItem[01];
            for ( INDEX ex = 0; ex < 5; ++ex )
            {
                DSADDR leadItem0Addr = getLinkAddress( pEntry[4] );
                if ( leadItem0Addr != 0 )
                    dumpMFDDirectoryLeadItem( stream, leadItem0Addr );
                pEntry += 5;
            }

            searchItemAddr = getLinkAddress( pSearchItem[0] );
        }
    }
}


//  dumpMFDDirectoryLeadItem()
//
//  Handles lead items for dumpMFDDirectory
void
MFDManager::dumpMFDDirectoryLeadItem
(
    std::ostream&       stream,
    const DSADDR        leadItem0Addr
)
{
    //  Read lead item 0 (and 1, if it exists)
    DSADDR leadItem1Addr = 0;
    Word36* pLeadItem0 = 0;
    Word36* pLeadItem1 = 0;
    Result result;
    if ( !stageLeadItems( leadItem0Addr, false, &leadItem1Addr, &pLeadItem0, &pLeadItem1, &result ) )
    {
        stream << "    Error reading lead item:" << getResultString( result ) << std::endl;
        return;
    }

    SuperString qualifier = miscWord36FieldataToString( &pLeadItem0[1], 2 );
    qualifier.trimTrailingSpaces();
    SuperString filename = miscWord36FieldataToString( &pLeadItem0[3], 2 );
    filename.trimTrailingSpaces();
    SuperString projectId = miscWord36FieldataToString( &pLeadItem0[5], 2 );
    projectId.trimTrailingSpaces();

    SuperString readKey("<none>");
    if ( !pLeadItem0[7].isZero() )
    {
        readKey = miscWord36FieldataToString( &pLeadItem0[7], 1 );
        readKey.trimTrailingSpaces();
    }

    SuperString writeKey("<none>");
    if ( !pLeadItem0[010].isZero() )
    {
        writeKey = miscWord36FieldataToString( &pLeadItem0[010], 1 );
        writeKey.trimTrailingSpaces();
    }

    const FileType fileType = ( pLeadItem0[011].getS1() == 0 ) ? FILETYPE_MASS_STORAGE : FILETYPE_TAPE; //TODO:REMO removable issue?
    const COUNT currentCycles = pLeadItem0[011].getS2();
    const COUNT maxRange = pLeadItem0[011].getS3();
    const COUNT currentRange = pLeadItem0[011].getQ3();
    const UINT16 highestAbsolute = pLeadItem0[011].getQ4();

    UINT32 statusBits = pLeadItem0[012].getT1();
    bool guardedSet = ( statusBits & 04000 ) ? true : false;
    bool plusOneExists = ( statusBits & 02000 ) ? true : false;
    bool nameChangeInProgress = ( statusBits & 01000 ) ? true : false;

    stream << "    FileSet " << qualifier << "*" << filename << "  ProjectId:" << projectId << std::endl;
    stream << "      ReadKey:" << readKey << "  WriteKey:" << writeKey
        << "  FileType:" << getFileTypeString( fileType ) << std::endl;
    stream << "      Cycles existing:" << currentCycles
        << "  maxRange:" << maxRange
        << "  currentRange:" << currentRange
        << "  highestAbsolute:" << highestAbsolute << std::endl;
    stream << "      Guarded:" << ( guardedSet ? "Yes" : "No" )
        << "  +1Exists:" << ( plusOneExists ? "Yes" : "No" )
        << "  nameChgInProg:" << ( nameChangeInProgress ? "Yes" : "No" ) << std::endl;

    Word36* pLink = &pLeadItem0[013];
    int relativeCycle = plusOneExists ? 1 : 0;
    for ( INDEX ex = 0; ex < currentRange; ++ex )
    {
        if ( ex == 17 )
            pLink = &pLeadItem1[01];
        DSADDR mainItem0Addr = getLinkAddress( pLink[0] );
        if ( mainItem0Addr != 0 )
            dumpMFDDirectoryMainItem( stream, fileType, relativeCycle, mainItem0Addr );
        ++pLink;
        --relativeCycle;
    }
}


//  dumpMFDDirectoryMainItem()
//
//  Handles main items for dumpMFDDirectory
void
MFDManager::dumpMFDDirectoryMainItem
(
    std::ostream&               stream,
    const FileType              fileType,
    const int                   relativeCycle,
    const DSADDR                mainItem0Addr
)
{
    Result result;

    DSADDR mainItem1Addr = 0;
    Word36* pMainItem0 = 0;
    Word36* pMainItem1 = 0;
    if ( !stageMainItems( mainItem0Addr, false, &mainItem1Addr, &pMainItem0, &pMainItem1, &result ) )
    {
        stream << "    Error reading main items:" << getResultString( result ) << std::endl;
        return;
    }

    SuperString qualifier = miscWord36FieldataToString( &pMainItem0[01], 2 );
    SuperString fileName = miscWord36FieldataToString( &pMainItem0[03], 2 );
    SuperString projectId = miscWord36FieldataToString( &pMainItem0[05], 2 );
    SuperString account = miscWord36FieldataToString( &pMainItem0[07], 2 );
    qualifier.trimTrailingSpaces();
    fileName.trimTrailingSpaces();
    projectId.trimTrailingSpaces();
    account.trimTrailingSpaces();

    UINT8 disableFlags = pMainItem0[013].getS1();
    bool directoryDisabled = ( disableFlags & 020 ) ? true : false;
    bool writeDisabled = ( disableFlags & 010 ) ? true : false;
    bool backupDisabled = ( disableFlags & 004 ) ? true : false;
    bool cacheDisabled = ( disableFlags & 002 ) ? true : false;

    UINT16 descriptorFlags = pMainItem0[014].getT1();
    bool unloaded = ( descriptorFlags & 04000 ) ? true : false;
    bool backedUp = ( descriptorFlags & 02000 ) ? true : false;
    bool saveOnCheckpoint = ( descriptorFlags & 01000 ) ? true : false;
    bool toBeCataloged = ( descriptorFlags & 00100 ) ? true : false;
    bool tapeFile = ( descriptorFlags & 00040 ) ? true : false;
    bool toBeWriteOnly = ( descriptorFlags & 00004 ) ? true : false;
    bool toBeReadOnly = ( descriptorFlags & 00002 ) ? true : false;
    bool toBeDropped = ( descriptorFlags & 00001 ) ? true : false;

    UINT8 fileFlags = pMainItem0[014].getS3();
    bool largeFile = ( fileFlags & 040 ) ? true : false;
    bool writtenTo = ( fileFlags & 002 ) ? true : false;
    bool storeThrough = ( fileFlags & 001 ) ? true : false;

    UINT8 pCharFlags = pMainItem0[015].getS1();
    bool positionGranularity = ( pCharFlags & 040 ) ? true : false;
    bool wordAddressable = ( pCharFlags & 010 ) ? true : false;
    bool asgToCommonName = ( pCharFlags & 004 ) ? true : false;

    SuperString asgMnemonic = miscWord36FieldataToString( &pMainItem0[016], 1 );
    asgMnemonic.trimTrailingSpaces();

    COUNT cumulativeAssignCount = pMainItem0[017].getH2();

    UINT8 inhibitFlags = pMainItem0[021].getS2();
    bool guarded = ( inhibitFlags & 040 ) ? true : false;
    bool unloadInhibit = ( inhibitFlags & 020 ) ? true : false;
    bool privateFile = ( inhibitFlags & 010 ) ? true : false;
    bool exclusiveUse = ( inhibitFlags & 004 ) ? true : false;
    bool writeOnly = ( inhibitFlags & 002 ) ? true : false;
    bool readOnly = ( inhibitFlags & 001 ) ? true : false;

    COUNT currentAssignCount = pMainItem0[021].getT2();
    UINT16 absoluteCycle = pMainItem0[021].getT3();

    stream << "      " << qualifier << "*" << fileName
        << "(" << std::dec << absoluteCycle << ")"
        << "/(" << ( relativeCycle > 0 ? "+" : "-" ) << relativeCycle << ")"
        << "  Project:" << projectId
        << "  Account:" << account
        << "  AsgTDate:" << TDate( pMainItem0[022] )
        << "  CatTDate:" << TDate( pMainItem0[023] )
        << std::endl;

    stream << "        AsgMnem:" << asgMnemonic
        << "  CumulAsg:" << cumulativeAssignCount
        << "  CurrentAsg:" << currentAssignCount
        << " - Inhibits:"
        << ( guarded ? "GUARD " : "" )
        << ( unloadInhibit ? "UNLD " : "" )
        << ( privateFile ? "PRIV " : "" )
        << ( exclusiveUse ? "XUSE " : "" )
        << ( writeOnly ? "WRONLY " : "" )
        << ( readOnly ? "RDONLY " : "" )
        << std::endl;

    stream << "        Desc:"
        << ( unloaded ? "UNLD " : "" )
        << ( backedUp ? "BKUP " : "" )
        << ( saveOnCheckpoint ? "SCHKPT " : "" )
        << ( toBeCataloged ? "ToBeCAT " : "" )
        << ( tapeFile ? "TAPE " : "" )
        << ( toBeReadOnly ? "ToBeRDONLY " : "" )
        << ( toBeWriteOnly ? "ToBeWRONLY " : "" )
        << ( toBeDropped ? "ToBeDRP " : "" );

    stream << " - FileFlg:"
        << ( largeFile ? "LARGE " : "" )
        << ( writtenTo ? "WRITTEN " : "" )
        << ( storeThrough ? "STHRU " : "" );

    stream << " - PCHAR:"
        << ( positionGranularity ? "POSGRN " : "" )
        << ( wordAddressable ? "WADDR " : "" )
        << ( asgToCommonName ? "ASGCNS " : "" );

    if ( disableFlags )
    {
        stream << " - Disbl: "
            << ( directoryDisabled ? "MFD " : "" )
            << ( writeDisabled ? "Write " : "" )
            << ( backupDisabled ? "Backup " : "" )
            << ( cacheDisabled ? "Cache " : "" );
    }

    stream << std::endl;

    //TODO:REM is there a fileType for removable?
    if ( fileType == FILETYPE_MASS_STORAGE )
    {
        COUNT initialReserve = pMainItem0[024].getH1();
        COUNT maxGranules = pMainItem0[025].getH1();
        COUNT highestGranuleAssigned = pMainItem0[026].getH1();
        COUNT highestTrackWritten = pMainItem0[027].getH1();
        stream << "        InitGran:" << initialReserve
            << "  MaxGran:" << maxGranules
            << "  HighestGran:" << highestGranuleAssigned
            << "  HighestTrk:" << highestTrackWritten
            << std::endl;

        //  Chase DAD tables
        DSADDR dadAddr = getLinkAddress( pMainItem0[0] );
        if ( dadAddr == 0 )
            stream << "        No DAD Sectors" << std::endl;
        else
        {
            stream << "        DAD Sector  FileRelative  Limit         DeviceRelatv  Region        Entry   Device" << std::endl;
            stream << "        DSADDR      Word Address  Word Address  Word Address  Word Length   Flags   Index " << std::endl;
            stream << "        ----------  ------------  ------------  ------------  ------------  ------  ------" << std::endl;

            while ( dadAddr != 0 )
            {
                Word36* pDAD;
                if ( !stageDirectorySector( dadAddr, false, &pDAD, &result ) )
                {
                    stream << "        MFD ERROR reading DAD at DSADDR=0" << std::oct << dadAddr << std::endl;
                    stream << "        Result:" << getResultString( result ) << std::endl;
                    break;
                }

                UINT64 regionAddr = pDAD[2].getW();
                UINT64 limitAddr = pDAD[3].getW();
                stream << "        "
                    << std::oct << std::setw( 10 ) << std::setfill( '0' ) << dadAddr
                    << "  "
                    << std::oct << std::setw( 12 ) << std::setfill( '0' ) << regionAddr
                    << "  "
                    << std::oct << std::setw( 12 ) << std::setfill( '0' ) << limitAddr
                    << std::endl;

                Word36* pEntry = pDAD + 04;
                for ( INDEX ex = 0; ex < 8; ++ex )
                {
                    //  Safety check in case last DAD entry flag isn't there...
                    if ( regionAddr >= limitAddr )
                    {
                        stream << "        WARNING:No last DAD flag found!" << std::endl;
                        break;
                    }

                    UINT64 deviceWordAddr = pEntry[0].getW();
                    UINT64 wordLength = pEntry[1].getW();
                    UINT32 dadFlags = pEntry[2].getH1();
                    UINT32 deviceIndex = pEntry[2].getH2();

                    stream << "                                                "
                        << std::oct << std::setw( 12 ) << std::setfill( '0' ) << deviceWordAddr
                        << "  "
                        << std::oct << std::setw( 12 ) << std::setfill( '0' ) << wordLength
                        << "  "
                        << std::oct << std::setw( 6 ) << std::setfill( '0' ) << dadFlags
                        << "  "
                        << std::oct << std::setw( 6 ) << std::setfill( '0' ) << deviceIndex
                        << std::endl;

                    //  Is this the last DAD entry?
                    if ( dadFlags & 04 )
                        break;
                    pEntry += 3;
                }

                dadAddr = getLinkAddress( pDAD[0] );
            }
        }
    }

    if ( fileType == FILETYPE_TAPE )
    {
        UINT8 density = pMainItem0[024].getS1();
        UINT8 format = pMainItem0[024].getS2();
        COUNT reels = pMainItem0[024].getH2();
        UINT8 mtapop = pMainItem0[025].getS3();
        UINT8 noise = pMainItem0[025].getS6();
        stream << "        Density:";
        switch ( density )
        {
        case 01:
            stream << "800/38000BPI";
            break;
        case 02:
            stream << "1600BPI";
            break;
        case 03:
            stream << "6250BPI";
            break;
        case 04:
            stream << "5090BPMM";
            break;
        case 05:
            stream << "38000BPI";
            break;
        case 06:
            stream << "76000BPI";
            break;
        case 07:
            stream << "85937BPI";
            break;
        default:
            stream << std::oct << density;
        }

        stream << " Format:"
            << (format & 01 ? "DataCnv " : "")
            << (format & 02 ? "QwPack " : "")
            << (format & 04 ? "6btPack " : "")
            << (format & 010 ? "8btPack " : "")
            << (format & 020 ? "Even " : "Odd ")
            << (format & 040 ? "9Trk " : "");

        stream << " Reels:" << reels;

        stream << " MTAPOP:"
            << (mtapop & 040 ? "CATJ " : "")
            << (mtapop & 020 ? "QIC " : "")
            << (mtapop & 010 ? "HIC " : "")
            << (mtapop & 004 ? "BufOff " : "")
            << (mtapop & 002 ? "DLT " : "")
            << (mtapop & 001 ? "HIS " : "");

        stream << " Noise:" << noise << std::endl;

        // TODO:TAPE display reels, possibly chasing reel tables
    }

    if ( !pMainItem1[010].isZero() )
    {
        UINT8 fasBits = pMainItem1[011].getS2();
        bool unloadedAtBackup = (fasBits & 040) ? true : false;
        bool numberBlocksPositions = (fasBits & 020) ? true : false;

        stream << "        BkupTDATE:" << TDate( pMainItem1[010] )
            << " Wrds:" << pMainItem1[07].getT1()
            << " MaxLvls:" << pMainItem1[011].getS1()
            << " CurLvls:" << pMainItem1[011].getS3()
            << " UnldBkup:" << (unloadedAtBackup ? "YES" : "NO")
            << " Blks:" << pMainItem1[011].getH2()
            << (numberBlocksPositions ? "POS" : "TRK")
            << " FilePos:" << pMainItem1[012].getW()
            << std::endl;
    }

    switch ( pMainItem1[017].getS2() )
    {
    case 1:
        stream << "        Leg 1 of TIP duplex file" << std::endl;
        break;
    case 2:
        stream << "        Leg 2 of TIP dupliex file" << std::endl;
        break;
    case 3:
        stream << "        TIP simplex file" << std::endl;
        break;
    }
}


//  establishLookupEntry()
//
//  Establishes a lookup entry for the given qual/file combination.
//
//  Parameters:
//      pActivity:          pointer to the invoking Activity
//      pQualifier:         pointer to 2-entry Word36 buffer containing LJSF qualfier in fieldata
//      pFileName:          pointer to 2-entry Word36 buffer containing LJSF filename in fieldata
//      leadItemAddr:       DSADDR of lead item sector 0 for the fileset
MFDManager::Result
    MFDManager::establishLookupEntry
    (
    Activity* const             pActivity,
    const Word36* const         pQualifier,
    const Word36* const         pFileName,
    const DSADDR                leadItemAddr
    )
{
    Result result;
    INDEX hashIndex = getLookupTableHashIndex( pQualifier, pFileName );
    LDATINDEX preferredLDATIndex = leadItemAddr >> 18;

    //  See if we can find an existing search item with an empty entry.
    //  Iterate over the existing search item chain (if there is one).
    DSADDR firstSearchItemAddr = m_SearchItemLookupTable[hashIndex];
    DSADDR lastSearchItemAddr = firstSearchItemAddr;
    Word36* pExisting = 0;
    if ( firstSearchItemAddr )
    {
        preferredLDATIndex = firstSearchItemAddr >> 18;

        //  We found a current lookup entry at the given hash index - walk the chain
        DSADDR searchItemAddr = firstSearchItemAddr;
        while ( searchItemAddr != 0 )
        {
            //  Get the search item
            lastSearchItemAddr = searchItemAddr;
            if ( !stageDirectorySector( searchItemAddr, false, &pExisting, &result ) )
                return result;

            Word36* pEntry = pExisting + 1;
            for ( INDEX32 ex = 0; ex < 5; ++ex )
            {
                DSADDR linkedAddr = getLinkAddress( pEntry[4] );
                if ( linkedAddr == 0 )
                {
                    //  We can use this item.  Update it appropriately.
                    pEntry[0] = pQualifier[0];
                    pEntry[1] = pQualifier[1];
                    pEntry[2] = pFileName[0];
                    pEntry[3] = pFileName[1];
                    pEntry[4] = leadItemAddr;

                    DEBUG_INSERT( searchItemAddr );
                    return result;
                }
                pEntry += 5;
            }

            //  This entry is full, look for another
            searchItemAddr = getLinkAddress( pExisting[0] );
        }
    }

    //  No empty search items (possibly no search items at all).
    //  Allocate a new directory sector, and build a new search item there-in.
    DSADDR newSearchItemAddr = 0;
    result = allocateDirectorySector( pActivity, preferredLDATIndex, &newSearchItemAddr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    Word36* pNewItem = 0;
    if ( !stageDirectorySector( newSearchItemAddr, true, &pNewItem, &result ) )
        return result;
    for ( INDEX wx = 0; wx < 28; ++wx )
        pNewItem[wx].setW( 0 );

    //  Set up new search item.
    pNewItem[0].setW( 0440000000000ll );
    if ( pExisting )
    {
        //  A chain already exists, and we have a pointer to the last search item in the chain.
        //  Link this new entry to the previous last existing item.
        pExisting[0].setW( newSearchItemAddr | 0040000000000ll );
        DEBUG_INSERT( lastSearchItemAddr );
    }
    else
    {
        //  No chain exists, so nothing to link to this new item.
        //  The lookup table needs to link here, though...
        m_SearchItemLookupTable[hashIndex] = newSearchItemAddr;
    }

    Word36* pEntry = &pNewItem[1];
    pEntry[0] = pQualifier[0];
    pEntry[1] = pQualifier[1];
    pEntry[2] = pFileName[0];
    pEntry[3] = pFileName[1];
    pEntry[4] = leadItemAddr;

    return result;
}


/*TODO:RECOV this *might* be used during recovery?  maybe?
//  getAllocatedDirectorySectors()
//
//  Populates a container with the DSADDR's of all the allocated directory sectors
//
//  Parameters:
//      pContainer:         pointer to container to be populated
//      includeDASs:        true to include DSADDR's of all the DAS sectors
MFDManager::Result
    MFDManager::getAllocatedDirectorySectors
    (
    std::set<DSADDR>* const     pContainer,
    const bool                  includeDASs
    )
{
    Result result;
    pContainer->clear();

    DSADDR dasAddr = 0;
    bool done = false;
    while ( !done )
    {
        //  Read the first/next DAS
        Word36* pDas = m_DataStoreCache.readDirectorySector( dasAddr, &result.m_DataStoreResult );
        if ( !pDas )
        {
            SystemLog::write( "MFDManager::getAllocatedDirectorySectors() cannot read DAS" );
            result.m_Status = MFDST_DATASTORE_ERROR;
            return result;
        }

        //  Iterate over the directory tracks described by this DAS
        for ( INDEX ex = 0; ex < 9; ++ex )
        {
            Word36* pEntry = &pDas[ex * 3];

            //  Is the track allocated?
            if ( (ex == 0) || (isLinkAddress( pEntry[0] )) )
            {
                //  Yeah, iterate over the bits in the 2-word bitmask, corresponding to sector addresses.
                DSADDR dsAddr = dasAddr + (ex * 64);
                UINT64 bitMask = (pEntry[1].getW() & 0777777777760) << 28;
                bitMask |= pEntry[2].getW() >> 4;

                //  Is the sector allocated, and do we want to know about it?
                for ( INDEX bx = 0; bx < 64; ++bx, bitMask <<= 1 )
                {
                    if ( (bitMask & 0x8000000000000000) && ( (dsAddr != dasAddr) || includeDASs ) )
                        pContainer->insert( dsAddr );
                }
            }
        }

        //  Move on to the next DAS (if there is one)
        if ( !isLinkAddress( pDas[27] ) )
            done = true;
        else
            dasAddr += 9 * 64;
    }

    return result;
}
*/


//  getConfigData()
//
//  (re)load data items which derive from configurator entries
void
    MFDManager::getConfigData()
{
    m_LookupTableSize = static_cast<COUNT32>(m_pExec->getConfiguration().getIntegerValue( "DCLUTS" ));
    m_OverheadAccountId = m_pExec->getConfiguration().getStringValue( "OVRACC" );
    m_OverheadUserId = m_pExec->getConfiguration().getStringValue( "OVRUSR" );
}


//  getFileSetInfo()
//
//  Populates a FileSetInfo object for the indicated lead item.
MFDManager::Result
MFDManager::getFileSetInfo
(
    const DSADDR            leadItemAddr0,
    FileSetInfo* const      pInfo
) const
{
    Result result;

    //  Read the lead item(s).
    DSADDR leadItemAddr1 = 0;
    Word36* pLeadItem0 = 0;
    Word36* pLeadItem1 = 0;
    if ( !stageLeadItems( leadItemAddr0, &leadItemAddr1, &pLeadItem0, &pLeadItem1, &result ) )
        return result;

    //  Start filling in the caller's FileSetInfo object
    pInfo->m_DSAddresses.resize( 2 );
    pInfo->m_DSAddresses[0] = leadItemAddr0;
    pInfo->m_DSAddresses[1] = leadItemAddr1;

    pInfo->m_MFDIdentifier = leadItemAddr0;
    pInfo->m_Qualifier = miscWord36FieldataToString( &pLeadItem0[01], 2 );
    pInfo->m_Qualifier.trimTrailingSpaces();
    pInfo->m_Filename = miscWord36FieldataToString( &pLeadItem0[03], 2 );
    pInfo->m_Filename.trimTrailingSpaces();
    pInfo->m_ProjectId = miscWord36FieldataToString( &pLeadItem0[05], 2 );
    pInfo->m_ProjectId.trimTrailingSpaces();
    if ( !pLeadItem0[07].isZero() )
    {
        pInfo->m_ReadKey = miscWord36FieldataToString( &pLeadItem0[07], 1 );
        pInfo->m_ReadKey.trimTrailingSpaces();
    }
    if ( !pLeadItem0[010].isZero() )
    {
        pInfo->m_WriteKey = miscWord36FieldataToString( &pLeadItem0[010], 1 );
        pInfo->m_WriteKey.trimTrailingSpaces();
    }

    pInfo->m_FileType = getFileType( pLeadItem0[011].getS1() );
    pInfo->m_MaxRange = pLeadItem0[011].getS3();
    pInfo->m_Guarded = (pLeadItem0[012].getT1() & 04000) != 0;
    pInfo->m_PlusOneCycleExists = (pLeadItem0[012].getT1() & 02000) != 0;

    //  Load FileCycleInfo objects
    UINT16 currentRange = pLeadItem0[011].getQ3();
    UINT16 highestAbsolute = pLeadItem0[011].getQ4();

    pInfo->m_CycleEntries.resize( currentRange );
    Word36* pEntry = &pLeadItem0[013];
    UINT16 thisAbsolute = highestAbsolute;
    INT16 thisRelative = pInfo->m_PlusOneCycleExists ? 1 : 0;
    for ( INDEX ex = 0; ex < currentRange; ++ex )
    {
        //  Entries 1-17 (ex 0:16) are in the first lead item; the remaining entries are in the second.
        //  We don't check to see whether sector 1 exists at this point; we assume that, if it does,
        //  we already followed the link from sector 0 to populate the buffer.
        if ( ex == 17 )
            pEntry = &pLeadItem1[01];

        //  If the entry is non-zero, there is an actual file cycle at this position
        if ( pEntry->getW() != 0 )
        {
            pInfo->m_CycleEntries[ex].m_Exists = true;
            if ( (pEntry->getW() & 0200000000000ll) != 0 )
                pInfo->m_CycleEntries[ex].m_ToBeCataloged = true;
            if ( (pEntry->getW() & 0100000000000ll) != 0 )
                pInfo->m_CycleEntries[ex].m_ToBeDropped = true;
            pInfo->m_CycleEntries[ex].m_RelativeCycle = thisRelative;
            pInfo->m_CycleEntries[ex].m_AbsoluteCycle = thisAbsolute;
            pInfo->m_CycleEntries[ex].m_MainItem0Addr = getLinkAddress( pEntry[0] );
            --thisRelative;
        }

        --thisAbsolute;
        if ( thisAbsolute == 0 )
            thisAbsolute = 999;
        ++pEntry;
    }

    return result;
}


//  getLeadItemAddress()
//
//  Retrieves the lead item address for a given qualifier/filename, if it exists.
DSADDR
MFDManager::getLeadItemAddress
(
    const Word36* const pQualifier,
    const Word36* const pFileName
) const
{
    //  Find the search item for the given qual/file combination, if it exists
    INDEX hx = getLookupTableHashIndex( pQualifier, pFileName );
    DSADDR lookupAddr = m_SearchItemLookupTable[hx];

    Word36* pLookupItem = 0;
    Word36* pEntry = 0;
    while ( lookupAddr != 0 )
    {
        //  Don't go overboard on this - if we don't find it, we don't find it.  Don't crash things.
        Result result;
        if ( !stageDirectorySector( lookupAddr, &pLookupItem, &result ) )
            return 0;

        pEntry = &pLookupItem[1];
        for ( INDEX ex = 0; ex < 5; ++ex )
        {
            if ( (pEntry[0].getW() == pQualifier[0].getW()) && (pEntry[1].getW() == pQualifier[1].getW())
                && (pEntry[2].getW() == pFileName[0].getW()) && (pEntry[3].getW() == pFileName[1].getW()) )
            {
                return getLinkAddress( pEntry[4] );
            }

            pEntry += 5;
        }

        lookupAddr = getLinkAddress( pLookupItem[0] );
    }

    return 0;
}


//  getSearchItemAddress()
//
//  Retrieves the search item address for a given qualifier/filename, if it exists.
//  For qualifiers and filenames in Fieldata LJSF format
inline DSADDR
MFDManager::getSearchItemAddress
(
    const Word36* const pQualifier,
    const Word36* const pFileName
) const
{
    //  Find the search item for the given qual/file combination, if it exists
    INDEX hx = getLookupTableHashIndex( pQualifier, pFileName );
    return m_SearchItemLookupTable[hx];
}


//  getSearchItemAddress()
//
//  Retrieves the search item address for a given qualifier/filename, if it exists.
//  For qualifiers and filenames held in SuperString objects
inline DSADDR
MFDManager::getSearchItemAddress
(
    const SuperString&  qualifier,
    const SuperString&  fileName
) const
{
    //  Convert superstrings to FD LJSF
    std::string tempQual = qualifier;
    tempQual.resize( 12, ' ' );
    std::string tempFile = fileName;
    tempFile.resize( 12, ' ' );

    Word36 fdQual[2];
    miscStringToWord36Fieldata( tempQual, fdQual, 2 );
    Word36 fdFile[2];
    miscStringToWord36Fieldata( tempFile, fdFile, 2 );

    //  Find the search item for the given qual/file combination, if it exists
    INDEX hx = getLookupTableHashIndex( fdQual, fdFile );
    return m_SearchItemLookupTable[hx];
}


//  initializeAssignMFD()
//
//  Assigns SYS$*MFDF$$ to the Exec.  For initial boots.
MFDManager::Result
MFDManager::initializeAssignMFD
(
    Activity* const             pActivity,
    const DSADDR                mfdMainItem0Addr,
    FileAllocationTable* const  pMFDFat             //  Pre-created FAT for MFD
)
{
    Result result;

    //  Find highest track for MFD - not hard to do.  Of course, we don't ever use it...
    CITDIRECTORYCACHE itLast = m_DirectoryCache.end();
    if ( itLast == m_DirectoryCache.begin() )
    {
        SystemLog::write("MFDManager::initializeAssignMFD() cache is empty");
        result.m_Status = MFDST_INTERNAL_ERROR; //  cache should never be empty at this point
        return result;
    }

    --itLast;
    DSADDR lastDSAddress = itLast->first;
    UINT32 highestTrack = static_cast<UINT32>( (lastDSAddress >> 6) & 077777777 );
    UINT32 highestGranule = highestTrack;

    DiskFacilityItem* pFacItem = new DiskFacilityItem( MFDFFileName,
                                                       MFDFQualifier,
                                                       ECODE_WORD_DISK,
                                                       OPTB_A | OPTB_X,
                                                       false,               //  release flag
                                                       0,                   //  absolute file cycle
                                                       false,               //  absolute file cycle (not) given
                                                       true,                //  exclusive assign
                                                       true,                //  existing file flag
                                                       mfdMainItem0Addr,
                                                       false,               //  read (not) inhibited
                                                       false,               //  read key (not) needed
                                                       0,                   //  relative file cycle
                                                       false,               //  relative file cycle (not) given
                                                       false,               //  temporary file flag
                                                       false,               //  write (not) inhibited
                                                       false,               //  write key (not) needed
                                                       pMFDFat,
                                                       MSGRAN_TRACK,
                                                       highestGranule,
                                                       highestTrack,
                                                       0,                   //  initial granules
                                                       0777777 );           //  max granules

    //  Now attach the FACitem to the RunInfo
    RunInfo* pRunInfo = pActivity->getRunInfo();
    pRunInfo->attach();
    pRunInfo->insertFacilityItem( pFacItem );
    pRunInfo->detach();

    //  Go back to the main item and set the assignment fields
    //  Main item 0[017].H2 is cumulative assign count
    //  Main item 0[021].T2 is current assign count
    //  Main item 0[021].bit9 is exclusive use flag
    //  Main item 0[022].W is TDATE$ current assignment started
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mfdMainItem0Addr, true, &pMainItem0, &result ) )
        return result;

    pMainItem0[017].setH2( 1 );
    pMainItem0[021].setT2( 1 );
    pMainItem0[021].logicalOr( 0000400000000L );

    TDate* pTDate = reinterpret_cast<TDate*>( &pMainItem0[022] );
    m_pExec->getExecTimeTDate( pTDate );

    DEBUG_INSERT( mfdMainItem0Addr );

    return result;
}


//  initializeCatalogMFD()
//
//  Creates an entry in the MFD, to describe the MFD.  Creates DAD entries as well.
//  Returns DSADDR of main item sector 0 to the caller through the parameter list.
MFDManager::Result
MFDManager::initializeCatalogMFD
(
    Activity* const             pActivity,
    DSADDR* const               pMainItem0Addr,
    FileAllocationTable** const ppMFDFat
)
{
    Result result;

    //  Catalog SYS$*MFDF$$ and create the DAD table in the MFD update cache.
    //  All updates go to MFD staged update area.
    //  Put the directory sectors on the first fixed device.
    LDATINDEX ldatIndex = 1;

    DSADDR searchItemAddr = 0;
    result = allocateDirectorySector( pActivity, ldatIndex, &searchItemAddr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    DSADDR leadItemAddr = 0;
    result = allocateDirectorySector( pActivity, ldatIndex, &leadItemAddr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    DSADDR mainItem0Addr = 0;
    result = allocateDirectorySector( pActivity, ldatIndex, &mainItem0Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    DSADDR mainItem1Addr = 0;
    result = allocateDirectorySector( pActivity, ldatIndex, &mainItem1Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    result = initializeCatalogMFDSearchItem( searchItemAddr, leadItemAddr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    result = initializeCatalogMFDLeadItem( leadItemAddr, mainItem0Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    result = initializeCatalogMFDMainItem0( mainItem0Addr, leadItemAddr, mainItem1Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    result = initializeCatalogMFDMainItem1( mainItem1Addr, mainItem0Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    result = initializeCreateMFDDADTable( pActivity, mainItem0Addr, ppMFDFat );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    *pMainItem0Addr = mainItem0Addr;
    return result;
}


//  initializeCatalogMFDLeadItem()
//
//  subfunction of initializeCatalogMFD() - we assume the sector buffer has been cleared.
inline MFDManager::Result
MFDManager::initializeCatalogMFDLeadItem
(
    const DSADDR        leadItem0Addr,
    const DSADDR        mainItem0Addr
)
{
    Result result;

    //  Find the sector
    Word36* pLeadItem = 0;
    if ( !stageDirectorySector( leadItem0Addr, true, &pLeadItem, &result ) )
        return result;

    pLeadItem[0].setW(0500000000000ll);         //  Descriptor and empty link address
    miscStringToWord36Fieldata( MFDFQualifier, &pLeadItem[1], 2 );
    miscStringToWord36Fieldata( MFDFFileName, &pLeadItem[3], 2 );
    miscStringToWord36Fieldata( m_OverheadUserId, &pLeadItem[5], 2 );   //  Project-ID field
    pLeadItem[011].setS2( 1 );                  //  number of f-cycles existing
    pLeadItem[011].setS3( 1 );                  //  maximum range of f-cycles
    pLeadItem[011].setQ3( 1 );                  //  current range
    pLeadItem[011].setQ4( 1 );                  //  highest absolute f-cycle
    pLeadItem[012].setT1( 04000 );              //  status bits - Guarded File
    pLeadItem[013].setW( mainItem0Addr );       //  link to main item 0 for the only cycle of the file

    return result;
}


//  initializeCatalogMFDMainItem0()
//
//  subfunction of initializeCatalogMFD() - we assume the sector buffer has been cleared.
inline MFDManager::Result
MFDManager::initializeCatalogMFDMainItem0
(
    const DSADDR        mainItem0Addr,
    const DSADDR        leadItemAddr,
    const DSADDR        mainItem1Addr
)
{
    Result result;

    //  Find the sector
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
        return result;

    TDate currentTDate;
    m_pExec->getExecTimeTDate( &currentTDate );

    pMainItem0[0].setW( 0600000000000ll );                                  //  DAD link - no link for now
    miscStringToWord36Fieldata( MFDFQualifier, &pMainItem0[1], 2 );         //  Qualifier
    miscStringToWord36Fieldata( MFDFFileName, &pMainItem0[3], 2 );          //  FileName
    miscStringToWord36Fieldata( m_OverheadUserId, &pMainItem0[5], 2 );      //  User-id for Project-id
    miscStringToWord36Fieldata( m_OverheadAccountId, &pMainItem0[7], 2 );   //  Account-id
    pMainItem0[015].setW( mainItem1Addr );                                  //  Link to main item sector 1
    miscStringToWord36Fieldata("F     ", &pMainItem0[016]);                 // asg mnemonic
    pMainItem0[013].setW( leadItemAddr );                                   //  Link back to lead item sector 0
    pMainItem0[017].setH2( 0 );                                             // cumulative assign count
    pMainItem0[021].setS2( 070 );                                           // inhibit flags guarded, inhibit rolout, private
    pMainItem0[021].setT2( 0 );                                             // current assigned count
    pMainItem0[021].setT3( 1 );                                             // absolute fcycle
    pMainItem0[022] = currentTDate;                                         // TDATE$ of current assign
    pMainItem0[023] = currentTDate;                                         // TDATE$ of catalog
    pMainItem0[024].setH1( 1 );                                             // init grans
    pMainItem0[025].setH1( 0777777 );                                       // max grans - max possible
    //  highest granule assigned, and highest track written are zero (H1 of 026 and 027).
    //  They're already zero, so...

    return result;
}


//  initializeCatalogMFDMainItem1()
//
//  subfunction of initializeCatalogMFD() - we assume the sector buffer has been cleared.
inline MFDManager::Result
MFDManager::initializeCatalogMFDMainItem1
(
    const DSADDR        mainItem1Addr,
    const DSADDR        mainItem0Addr
)
{
    Result result;

    //  Find the sector
    Word36* pMainItem1 = 0;
    if ( !stageDirectorySector( mainItem1Addr, true, &pMainItem1, &result ) )
        return result;

    pMainItem1[0].setW( 0400000000000 );
    miscStringToWord36Fieldata( MFDFQualifier, &pMainItem1[1], 2 );     //  Qualifier (again)
    miscStringToWord36Fieldata( MFDFFileName, &pMainItem1[3], 2 );      //  Filename (again)
    miscStringToWord36Fieldata( "*NO.1*", &pMainItem1[5], 1 );          //  Identifies the ordinal of this sector
    pMainItem1[6].setW( mainItem0Addr );                                //  Link back to main item sector 0
    pMainItem1[7].setT3( 1 );                                           // abs file cycle
    pMainItem1[011].setS1( 1 );                                         // one backup level allowed (does this matter?)

    return result;
}


//  initializeCatalogMFDSearchItem()
//
//  subfunction of initializeCatalogMFD()
inline MFDManager::Result
MFDManager::initializeCatalogMFDSearchItem
(
    const DSADDR        searchItemAddr,
    const DSADDR        leadItem0Addr
)
{
    Result result;

    //  Find the sector
    Word36* pSearchItem = 0;
    if ( !stageDirectorySector( searchItemAddr, true, &pSearchItem, &result ) )
        return result;

    //  Populate the sector with a single entry for SYS$*MFDF$$
    pSearchItem[0].setW( 0440000000000ll );
    miscStringToWord36Fieldata( MFDFQualifier, &pSearchItem[1], 2 );
    miscStringToWord36Fieldata( MFDFFileName, &pSearchItem[3], 2 );
    pSearchItem[5].setW( leadItem0Addr );

    //  Update lookup table to point to this sector
    INDEX hashIndex = getLookupTableHashIndex( &pSearchItem[1], &pSearchItem[3] );
    m_SearchItemLookupTable[hashIndex] = searchItemAddr;

    return result;
}


//  initializeCreateMFDDADTable
//
//  Creates a DAD table for the MFD.
//  We update the main item, so the caller must make sure it is marked updated.
//
//  If successful, we pass back a pointer to a newly-allocated FAT.
MFDManager::Result
MFDManager::initializeCreateMFDDADTable
(
    Activity* const             pActivity,
    const DSADDR                mainItem0Addr,
    FileAllocationTable** const ppMFDFat
)
{
    Result result;

    //  Create FileAllocationTable
    FileAllocationTable* pMFDFat = new FileAllocationTable( mainItem0Addr, false );

    //  Now iterate over all of the packs in the fixed pool
    for ( ITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        PackInfo* pPackInfo = itpi->second;
        //  Don't worry UP or SU... If it's in the fixed pool, it is UP or SU,
        //  and if the user tries to trip us up with a well-timed DN or RV keyin, screw him.
        //  In fact, maybe we should restrict those keyins during the initialization process...
        if ( pPackInfo->m_InFixedPool )
        {
            //  Stage sector 1 for the pack
            DSADDR sector0Addr = (pPackInfo->m_LDATIndex << 18);
            Word36* pSector0 = 0;
            if ( !stageDirectorySector( sector0Addr, false, &pSector0, &result ) )
                return result;

            DSADDR sector1Addr = sector0Addr + 1;
            Word36* pSector1 = 0;
            if ( !stageDirectorySector( sector1Addr, false, &pSector1, &result ) )
                return result;

            SECTOR_COUNT dasOffset = pSector1[020].getH2();
            if ( dasOffset != 0 )
            {
                //  Initial tracks are all contiguous, except (possibly) the first DAS track
                //  There is some ambiguity as to whether initial tracks count includes the first DAS track,
                //  so we avoid using that value.  Instead, we figure out how many directory tracks preceed the
                //  DAS track, based on the dasOffset value.
                TRACK_COUNT initTracks = dasOffset / SECTORS_PER_TRACK;
                TRACK_ID firstDiskDirectoryTrack = pPackInfo->m_DirectoryTrackAddress / SECTORS_PER_TRACK;
                TRACK_ID firstFileRelativeTrack = pPackInfo->m_LDATIndex << 12;
                pMFDFat->allocated( firstFileRelativeTrack, initTracks, pPackInfo->m_LDATIndex, firstDiskDirectoryTrack );

                //  Account for first DAS track
                DRWA firstDASDiskWord = pSector0[033].getW();
                TRACK_ID firstDiskDASTrack = firstDASDiskWord / WORDS_PER_TRACK;
                TRACK_ID firstFileDASTrack = firstFileRelativeTrack + initTracks;
                pMFDFat->allocated( firstFileDASTrack, 1, pPackInfo->m_LDATIndex, firstDiskDASTrack );
            }
            else
            {
                //  In this case, we have at most 9 directory tracks, which means they all fit in the sector 0 DAS.
                //  Account for the first (DAS) track.
                TRACK_ID deviceRelativeTrackId = pPackInfo->m_DirectoryTrackAddress / SECTORS_PER_TRACK;
                TRACK_ID fileRelativeTrackId = pPackInfo->m_LDATIndex << 12;
                pMFDFat->allocated( fileRelativeTrackId, 1, pPackInfo->m_LDATIndex, deviceRelativeTrackId );

                //  Account for subsequent (if any) tracks according to the DAS
                Word36* pEntry = &pSector0[3];
                for ( INDEX ex = 1; ex < 9; ++ex )
                {
                    ++fileRelativeTrackId;
                    if ( !pEntry->isNegative() )
                    {
                        DRWA directoryTrackWordId = pEntry->getW();
                        deviceRelativeTrackId = directoryTrackWordId / WORDS_PER_TRACK;
                        pMFDFat->allocated( fileRelativeTrackId, 1, pPackInfo->m_LDATIndex, deviceRelativeTrackId );
                    }

                    pEntry += 3;
                }
            }
        }
    }

    //  Update the DAD in the FAT, then write it to the MFD
    pMFDFat->synchronizeDADTables();

    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
        return result;

    result = writeDADUpdates( pActivity, pMFDFat );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        delete pMFDFat;
        pMFDFat = 0;
    }

    *ppMFDFat = pMFDFat;
    return result;
}


//  initializeFixedPack()
//
//  Initializes the fixed pack on the indicated device.
//  Clears out the MFD (if there is such) on the pack, copies HMBT to SMBT,
//  and allocates an empty directory track beyond the initial tracks.
//  All this is done in the cache, which must be committed at some point.
//  Most of this would not be necessary for a freshly-prepped pack, but we do it anyway.
//
//  Caller must comit MFD updates.
//  We do not stop the Exec on IO errors - caller must decide whether that is appropriate.
MFDManager::Result
MFDManager::initializeFixedPack
(
    Activity* const         pActivity,
    ITPACKINFOMAP           itPackInfo
)
{
    Result result;

    LDATINDEX ldatIndex = itPackInfo->first;
    PackInfo* pPackInfo = itPackInfo->second;

    //  Load initial directory tracks from the pack into directory cache.
    //  Start with the first directory track.
    BLOCK_ID firstBlockId = pPackInfo->m_DirectoryTrackAddress / SECTORS_PER_BLOCK( pPackInfo->m_PrepFactor );
    DSADDR sector0Addr = pPackInfo->m_LDATIndex << 18;
    DSADDR sector1Addr = sector0Addr + 1;

    result = loadDirectoryTrackIntoCache( pActivity, pPackInfo, sector0Addr, firstBlockId );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    //  Go find the next directory track (if any) - this depends on whether we are chasing DAS sectors.
    //  We determine this by checking sector1[020].H2:
    //      If zero, then sector0 is a DAS.
    //      Else it is the number of sectors up to, but not including, the first DAS
    //          (and thus can be used as the sector offset from sector 0, for the first DAS).
    Word36* pSector0 = 0;
    if ( !stageDirectorySector( sector0Addr, false, &pSector0, &result ) )
        return result;

    Word36* pSector1 = 0;
    if ( !stageDirectorySector( sector1Addr, true, &pSector1, &result ) )
        return result;

    UINT32 dasOffset = pSector1[020].getH2();
    if ( dasOffset == 0 )
    {
        //  First DAS is in sector 0 - we will need to chase track information in the DAS.
        //  However, we know that there are 9 or fewer tracks, so we don't have to chase DAS sectors.
        //  So we just have to iterate over 8 DAS track 3-word entries (since we already loaded the first one).
        for ( INDEX32 ex = 1; ex < 9; ++ex )
        {
            Word36* pEntry = pSector0 + (3 * ex);
            if ( !pEntry->isNegative() )
            {
                DSADDR dirSectorAddr = sector0Addr + (ex * SECTORS_PER_TRACK);
                DRWA dirTrackAddr = pEntry->getW();
                BLOCK_ID dirBlockId = dirTrackAddr * BLOCKS_PER_TRACK(pPackInfo->m_PrepFactor);
                result = loadDirectoryTrackIntoCache( pActivity, pPackInfo, dirSectorAddr, dirBlockId );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                    return result;
            }
        }

        //  Now build sector 0 according to rules for small disks
        buildSector0ForSmallDisks( pSector0,
                                   pPackInfo->m_DirectoryTrackAddress,
                                   pPackInfo->m_S0S1HMBTPadWords,
                                   pPackInfo->m_SMBTWords );
        DEBUG_INSERT( sector0Addr );
    }
    else
    {
        //  First DAS follows contiguous MBT directory tracks.
        //  Load the directory tracks.  Make sure we're talking about a multiple of 9 of them.
        TRACK_COUNT directoryTracks = pSector0[0].getS1();
        if ( directoryTracks % 9 )
            directoryTracks -= (directoryTracks % 9);
        BLOCK_ID blockId = firstBlockId + BLOCKS_PER_TRACK(pPackInfo->m_PrepFactor);
        DSADDR sectorAddr = sector0Addr + SECTORS_PER_TRACK;
        for ( TRACK_COUNT track = 1; track < directoryTracks; ++track )
        {
            result = loadDirectoryTrackIntoCache( pActivity, pPackInfo, sectorAddr, blockId );
            if ( result.m_Status != MFDST_SUCCESSFUL )
                return result;
            sectorAddr += SECTORS_PER_TRACK;
            blockId += BLOCKS_PER_TRACK(pPackInfo->m_PrepFactor);
        }

        //  Sector0 is formatted according to rules for large disks - this is not a DAS sector.
        //  It should be all zeroes except for +0,S1 which contains the
        //  number of initial directory tracks used to host S0/S1/HMBT/padding/SMBT, and +27,W
        //  which is the link to the first DAS track.  We don't need to change this.
        //  However, we DO need to initialize the DAS in the DAS track which it references.
        Word36* pDAS = new Word36[WORDS_PER_SECTOR];
        buildEmptyDASSector( pDAS, pPackInfo->m_LDATIndex );

        //  Now stage the DAS track - we don't actually load it, since we don't care what's in it.
        //  Additionally, some storage media might give us errors it that track hasn't yet been written.
        DSADDR dasSectorAddr = sector0Addr + dasOffset;
        TRACK_ID dasTrackId = dasSectorAddr >> 6;
        DRWA dasDeviceTrackWord = pSector0[033].getW();
        TRACK_ID dasDeviceTrackId = dasDeviceTrackWord / WORDS_PER_TRACK;
        BLOCK_ID dasBlockId = dasDeviceTrackId * BLOCKS_PER_TRACK(pPackInfo->m_PrepFactor);
        m_DirectoryTrackIdMap[dasTrackId] = dasBlockId;

        m_DirectoryCache[dasSectorAddr] = pDAS;
        for ( INDEX32 sx = 1; sx < SECTORS_PER_TRACK; ++sx )
            m_DirectoryCache[dasSectorAddr + sx] = new Word36[WORDS_PER_SECTOR];
    }

    //  Fix up sector 1.  update word 3 (current available tracks) from word 2 (max avail),
    //  and put LDAT Index in word 5,H1
    pSector1[3].setW( pSector1[2].getW() );
    pSector1[5].setH1( ldatIndex );

    //  Copy HMBT to SMBT, effectively releasing all allocated non-directory tracks (if any).
    DSADDR hmbtAddr = (ldatIndex << 18) | 2;
    DSADDR smbtAddr = (ldatIndex << 18) | static_cast<COUNT32>(pPackInfo->m_S0S1HMBTPadWords / 28);
    SECTOR_COUNT smbtSectors = pPackInfo->m_SMBTWords / 28;
    if ( pPackInfo->m_SMBTWords % 28 )
        ++smbtSectors;

    Word36* pHMBTSector = 0;
    Word36* pSMBTSector = 0;
    for ( SECTOR_COUNT sc = 0; sc < smbtSectors; ++sc )
    {
        if ( !stageDirectorySector( hmbtAddr, false, &pHMBTSector, &result ) )
            return result;
        if ( !stageDirectorySector( smbtAddr, true, &pSMBTSector, &result ) )
            return result;
        for ( INDEX wx = 0; wx < 28; ++wx )
            pSMBTSector[wx] = pHMBTSector[wx];
        ++smbtAddr;
        ++hmbtAddr;
    }

    return result;
}


//  initializeFixedPacks()
//
//  Called early in jk13 boot - after pack labels are read, LDATs are assigned,
//  m_PackInfo is populated, and initial directory sectors are loaded.
//  We cannot commit the consequent MFD updates yet, since the FAT isn't loaded for the MFD.
//  This means the caller must do this commit as soon as it is practical.
MFDManager::Result
MFDManager::initializeFixedPacks
(
    Activity* const         pActivity
)
{
    //  No lock required - this occurs early in boot
    Result result;
    for ( ITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        if ( itpi->second->m_IsFixed )
        {
            result = initializeFixedPack( pActivity, itpi );
            if ( result.m_Status != MFDST_SUCCESSFUL )
                break;
        }
    }

    return result;
}


//  initializeLoadPackInfo()
//
//  Iterates over the given list of PackInfo objects (which do not yet have LDAT's, so are not in m_PackInfo)
//  assigning LDATs to all the packs and building up m_PackInfo.
void
MFDManager::initializeLoadPackInfo
(
    Activity* const         pActivity,
    const PACKINFOLIST&     packInfoList
)
{
//????    DeviceManager* pdevmgr = dynamic_cast<DeviceManager*>( m_pExec->getManager( Exec::MID_DEVICE_MANAGER ) );
    LDATINDEX nextLDAT = 1;
    for ( CITPACKINFOLIST itpi = packInfoList.begin(); itpi != packInfoList.end(); ++itpi )
    {
        PackInfo* pPackInfo = *itpi;
        if ( pPackInfo->m_IsFixed )
        {
            //  This pack is fixed - is it in conflict with any other already-'registered' fixed pack?
            //  If so, mark it DN.
            bool conflict = false;
            for ( CITPACKINFOMAP itpiRegistered = m_PackInfo.begin(); itpiRegistered != m_PackInfo.end(); ++itpiRegistered )
            {
                PackInfo* pRegisteredPackInfo = itpiRegistered->second;
                if ( pPackInfo->m_PackName.compareNoCase( pRegisteredPackInfo->m_PackName ) == 0 )
                {
                    std::string consMsg = "DEVICE ";
                    consMsg += m_pDeviceManager->getNodeName( pPackInfo->m_DeviceId ) + " PACK-ID CONFLICT";
                    m_pConsoleManager->postReadOnlyMessage( consMsg, 0 );
                    m_pDeviceManager->setNodeDownInternally( pActivity, pPackInfo->m_DeviceId );
                    conflict = true;
                    break;
                }
            }

            if ( !conflict )
            {
                //  No conflict - assign an LDAT and integrate the pack into the fixed pool
                pPackInfo->m_LDATIndex = nextLDAT++;
                m_PackInfo[pPackInfo->m_LDATIndex] = pPackInfo;
                pPackInfo->m_InFixedPool = true;
                pPackInfo->m_AssignCount = 1;

                //  Notify listeners
                /*???? obsolete, but keep it around in case
                const DeviceManager::NodeEntry* pNodeEntry = pdevmgr->getNodeEntry( pPackInfo->m_DeviceId );
                m_pExec->postNodeChangeEvent( pNodeEntry->m_pNode );
                */
            }
        }
    }
}


//  insertMainItem()
//
//  Inserts a main item link into the appropriate lead item sector,
//  according to the indicated absolute file cycle.
//
//  Parameters:
//      pActivity:          pointer to the controlling Activity
//      leadItem0Addr:      DSADDR of lead item into which the link is to be inserted
//      mainItem0Addr:      DSADDR link to main item to be inserted into the lead item
//      absoluteCycle:      absolute file cycle associated with the relevant main item
//      guardedFile:        true if main item represents a guarded file cycle (which forces the whole set to be guarded)
MFDManager::Result
MFDManager::insertMainItem
(
    Activity* const         pActivity,
    const DSADDR            leadItem0Addr,
    const DSADDR            mainItem0Addr,
    const UINT16            absoluteCycle,
    const bool              guardedFile
)
{
    Result result;

    //  Read lead items 0 and optionally 1
    Word36* pLeadItem0 = 0;
    Word36* pLeadItem1 = 0;
    DSADDR leadItem1Addr = 0;
    if ( !stageLeadItems( leadItem0Addr, true, &leadItem1Addr, &pLeadItem0, &pLeadItem1, &result ) )
        return result;

    //  Update lead items 0 and possibly 1.  If the fileset is empty, this is pretty simple.
    COUNT currentCycleRange = pLeadItem0[011].getQ3();

    //  If this cycle is guarded, the whole set becomes guarded.
    if ( guardedFile )
        pLeadItem0[012].logicalOr( 0400000000000ll );

    if ( currentCycleRange == 0 )
    {
        //  Empty file set - this is the only cycle that will exist in the file set.
        pLeadItem0[011].setS2( 1 );
        pLeadItem0[011].setQ3( 1 );
        pLeadItem0[011].setQ4( absoluteCycle );
        pLeadItem0[013].setW( mainItem0Addr );
    }
    else
    {
        //  Non-empty file set - we have to maneuver around other existing file cycles.
        //  Calculate the new cycle range - it might be different from the current, and the exact calculation
        //  depends on whether the new cycle is higher or lower than the current absolute max cycle.
        UINT16 currentHighestAbsoluteCycle = pLeadItem0[011].getQ4();
        COUNT newCycleRange = 0;
        UINT16 compRange = execGetAbsoluteCycleRange( absoluteCycle, currentHighestAbsoluteCycle );   //  range from current highest to new abs cycle
        int comparison = execCompareAbsoluteCycles( absoluteCycle, currentHighestAbsoluteCycle );   //  comparison from new cycle to current highest
        if ( comparison > 0 )
        {
            //  new cycle sorts higher than any existing cycles, so the range *will* be extended.
            newCycleRange = currentCycleRange + compRange - 1;
        }
        else
        {
            //  new cycle sorts lower than the highest existing cycle, so the range *might* be extended.
            newCycleRange = compRange > currentCycleRange ? compRange : currentCycleRange;
        }

        //  If the new cycle range is different than the existing cycle range, update lead item 0.
        //  Then, if the original range didn't require a sector 1, and the new one does,
        //  create and link in a new lead item sector 1.
        if ( newCycleRange != currentCycleRange )
        {
            pLeadItem0[011].setQ3( newCycleRange );

            if ( (newCycleRange > 17) && (leadItem1Addr == 0) )
            {
                LDATINDEX preferredLDATIndex = leadItem0Addr >> 18;
                result = allocateDirectorySector( pActivity, preferredLDATIndex, &leadItem1Addr );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                    return result;

                pLeadItem1 = 0;
                if ( !stageDirectorySector( leadItem1Addr, false, &pLeadItem1, &result ) )
                    return result;

                //  Link item 1 to item 0 and clear item 1's forward link
                pLeadItem0[0].setW( leadItem1Addr );
                pLeadItem1[0].setW( 0400000000000 );
            }
        }

        if ( comparison > 0 )
        {
            //  The requested new cycle is higher than the current highest cycle.
            //  All the existing links will need to be moved downward in the lead items.
            //  We TRUST that calling code has already dropped cycles needing to be dropped,
            //  and that this procedure will not produce results inconsistent with max range.
            INDEX sx = currentCycleRange;
            INDEX dx = sx + compRange - 1;
            while ( sx > 0 )
            {
                --sx;
                --dx;
                {//TODO:DEBUG
                    std::cout << "Moving " << sx << " to " << dx << std::endl;
                }
                Word36* pSource = getMainItemLinkPointer( sx, pLeadItem0, pLeadItem1 );
                Word36* pDest = getMainItemLinkPointer( dx, pLeadItem0, pLeadItem1 );
                *pDest = *pSource;
                pSource->clear();
            }

            //  Now create the new link and update the highest absolute cycle
            pLeadItem0[013].setW( mainItem0Addr );
            pLeadItem0[011].setQ4( absoluteCycle );
        }
        else
        {
            //  The requested new cycle sorts lower than the current highest cycle.
            //  Find the exact location for the main item link.
            Word36* pEntry = getMainItemLinkPointer( compRange - 1, pLeadItem0, pLeadItem1 );
            pEntry->setW( mainItem0Addr );
        }

        //  Update number of actual files - we don't have to worry about the actual number,
        //  we just know we're increasing it by 1.
        pLeadItem0[011].setS2( pLeadItem0[011].getS2() + 1 );
    }

    return result;
}


//  internalPrep()
//
//  Preps a pack as fixed or removable.
//  Use device's blocksize to determine prepfactor, and blockcount to determine tracks.
//  Put first directory track about 1/3 of the way through the pack (by convention).
MFDManager::Result
MFDManager::internalPrep
(
    Activity* const                 pActivity,
    const DeviceManager::DEVICE_ID  deviceId,
    const bool                      fixedFlag,
    const std::string&              packLabel
)
{
    Result result;

    //  Obtain device info - if we can't get it, we cannot prep the thing.
    BLOCK_COUNT packBlockCount;
    BLOCK_SIZE packBlockSize;
    result = getDiskDeviceValues( pActivity, deviceId, &packBlockCount, &packBlockSize );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    PREP_FACTOR prepFactor = miscGetPrepFactorFromBlockSize( packBlockSize );
    if ( (prepFactor == 0) || (packBlockCount == 0) )
    {
        result.m_Status = MFDST_PACK_NOT_FORMATTED;
        return result;
    }

    const DeviceManager::DeviceEntry* pDevEntry = m_pDeviceManager->getDeviceEntry( deviceId );
    if ( pDevEntry == 0 )
    {
        std::stringstream strm;
        strm << "MFDManager::internalPrep() Cannot get device entry for device ID " << deviceId;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    SuperString adjustedPackLabel = packLabel;
    adjustedPackLabel.resize( 6, ' ' );
    adjustedPackLabel.foldToUpperCase();

    std::stringstream consStrm;
    consStrm << "PREP " << pDevEntry->m_pNode->getName()
        << (fixedFlag ? " FIX" : " REM")
        << " BSZ=" << packBlockSize
        << " BLKS=" << packBlockCount
        << " PREP=" << prepFactor
        << " LBL=" << packLabel;
    m_pConsoleManager->postReadOnlyMessage( consStrm.str(), 0 );

    //  Traditionally, we have used the first 2 physical blocks for booting and such,
    //  with the third reserved for the pack label.  This does not vary based on the prepfactor.
    //  (In fact, it is said that the first two physical blocks /can/ vary in size).
    //  To observe tradition, we'll make sure to reserve at least those 3 physical blocks
    //  in the hardware master bit table... but we need to know how many tracks we're talking about.
    TRACK_COUNT reservedTracks = (prepFactor == 1792) ? 3 : (prepFactor == 896) ? 2 : 1;
    COUNT recordsPerTrack = BLOCKS_PER_TRACK(prepFactor);
    COUNT sectorsPerBlock = SECTORS_PER_BLOCK(prepFactor);
    TRACK_COUNT totalTracks = ( packBlockCount * prepFactor ) / 1792;
    SECTOR_ID firstDirectoryTrackAddr = ( totalTracks / 3 ) * SECTORS_PER_TRACK;

    //  Calculate mbtLength - this is the length of an MBT, including the 2 delimiting words.
    WORD_COUNT mbtLength = totalTracks / 32;
    if ( totalTracks % 32 )
        ++mbtLength;
    mbtLength += 2;
    SECTOR_COUNT smbtSectors = mbtLength / WORDS_PER_SECTOR;
    if ( mbtLength % WORDS_PER_SECTOR )
        ++mbtLength;

    //  Calculate hbmtLength - this is the length of S0+S1+HMBT+Padding_to_block_boundary in words.
    WORD_COUNT hmbtLength = (2 * WORDS_PER_SECTOR) + mbtLength;
    if ( hmbtLength % prepFactor )
        hmbtLength = ( ( hmbtLength / prepFactor ) + 1 ) * prepFactor;
    SECTOR_COUNT hmbtSectors = hmbtLength / WORDS_PER_SECTOR;
    if ( hmbtLength % WORDS_PER_SECTOR )
        ++hmbtSectors;

    //  Calculate initial track allocation.
    //  First, determine number of directory tracks.  If >= 9, then it must be a multiple of 9.
    //  Then determine the number of initial tracks (directory tracks + track containing VOL1).
    //  Finally, the number of tracks remaining after initial allocation.
    TRACK_COUNT directoryTracks = (hmbtLength + mbtLength) / WORDS_PER_TRACK;
    if ( (hmbtLength + mbtLength) % WORDS_PER_TRACK )
        ++directoryTracks;
    //TODO:DEBUG BELOW IS A BUG - LEAVE FOR NOW, TO TEST LARGE DISK ALGORITHMS
    if ( directoryTracks % 9 )
        directoryTracks = ( ( directoryTracks / 9 ) + 1 ) * 9;

    TRACK_COUNT initialTracks = directoryTracks + reservedTracks + (directoryTracks >= 9) ? 1 : 0;
    TRACK_COUNT remainingTracks = totalTracks - initialTracks;

    //  If this is a large disk (i.e., 9 or more directory tracks) create a DAS track
    //  beyond what will become the initial directory tracks.
    if ( directoryTracks >= 9 )
    {
        DRWA dasTrackWordAddress = (firstDirectoryTrackAddr * 28) + (directoryTracks * 1792);
        result = internalPrepCreateDASSector( pActivity, deviceId, prepFactor, dasTrackWordAddress );
        if ( result.m_Status != MFDST_SUCCESSFUL )
            return result;
    }

    //  Build initial directory tracks in-memory.  This takes a nice chunk of memory.
    //  Note that the initial directory tracks (and the first DAS track for large disks)
    //  are placed in consecutive physical blocks.
    Word36* pDirectorySectors = new Word36[WORDS_PER_TRACK * static_cast<COUNT>(directoryTracks)];

    //  Build Sector 0
    Word36* pSector0 = pDirectorySectors;
    internalPrepBuildSector0( pSector0, firstDirectoryTrackAddr, directoryTracks, hmbtLength, mbtLength );

    //  Build Sector 1 (always version 1)
    COUNT dasOffset = pSector0[0].getS1() * SECTORS_PER_TRACK;
    Word36* pSector1 = pDirectorySectors + WORDS_PER_SECTOR;
    internalPrepBuildSector1( pSector1, hmbtSectors, smbtSectors, remainingTracks, adjustedPackLabel, mbtLength, dasOffset );

    //  Fill-in HMBT
    Word36* pHMBT = pDirectorySectors + (2 * WORDS_PER_SECTOR);
    internalPrepBuildMBT( pHMBT, totalTracks, reservedTracks, directoryTracks, firstDirectoryTrackAddr, mbtLength );

    //  Fill-in SMBT (just copy from HMBT)
    Word36* pSMBT = pDirectorySectors + (hmbtSectors * WORDS_PER_SECTOR);
    for ( INDEX wx = 0; wx < (smbtSectors * WORDS_PER_SECTOR); ++wx )
        pSMBT[wx] = pHMBT[wx];

    //  Write initial directory tracks
    BLOCK_ID directoryBlockId = firstDirectoryTrackAddr / sectorsPerBlock;
    BLOCK_COUNT blocksPerTrack = BLOCKS_PER_TRACK(prepFactor);
    BLOCK_COUNT directoryBlocks = directoryTracks * blocksPerTrack;

    Word36* pWriteBlock = pDirectorySectors;
    BLOCK_ID writeBlockId = directoryBlockId;
    for ( COUNT blockCount = 0; blockCount < directoryBlocks; ++blockCount )
    {
        result = directDiskIo( pActivity, deviceId, ChannelModule::Command::WRITE, writeBlockId, prepFactor, pWriteBlock );
        if ( result.m_Status != MFDST_SUCCESSFUL )
            return result;

        pWriteBlock += prepFactor;
        ++writeBlockId;
    }

    delete[] pDirectorySectors;
    pDirectorySectors = 0;
    pWriteBlock = 0;

    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    //  Create VOL1 label
    Word36* pLabel = new Word36[prepFactor];
    miscStringToWord36Ascii( "VOL1", &pLabel[0] );
    miscStringToWord36Ascii( adjustedPackLabel, &pLabel[1] );
    pLabel[3].setW( firstDirectoryTrackAddr );
    pLabel[4].setH1( recordsPerTrack );
    pLabel[4].setH2( prepFactor );
    pLabel[011].setH1( static_cast<COUNT>(hmbtLength) );
    pLabel[011].setH2( static_cast<COUNT>(mbtLength) );
    pLabel[016].setW( totalTracks );

    //  Write VOL1 label
    result = directDiskIo( pActivity, deviceId, ChannelModule::Command::WRITE, 2, prepFactor, pLabel );

    delete[] pLabel;
    return result;
}


//  internalPrepCreateDASSector()
//
//  For internal prep of large disk drives (which require 9 or more initial directory tracks)
//  we go ahead and create an empty DAS directory track following the initial directory tracks.
MFDManager::Result
MFDManager::internalPrepCreateDASSector
(
    Activity* const                 pActivity,
    const DeviceManager::DEVICE_ID  deviceId,
    const PREP_FACTOR               prepFactor,
    const DRWA                      dasTrackWordAddress
) const
{
    Result result;
    Word36* pBuffer = new Word36[prepFactor];
    Word36* pDAS = pBuffer;

    //  Word 0 S1 is 02 for removable, 03 for fixed (we're doing fixed)
    //  The remainder of the word is the sector address of the DAS
    pDAS[0].setW( dasTrackWordAddress / 28 );
    pDAS[0].setS1( 03 );

    //  Only the first sector (the DAS sector) is used - set the flag for it.
    pDAS[1].setW( 0400000000000 );
    for ( INDEX ex = 1; ex < 9; ++ex )
    {
        Word36* pEntry = &pDAS[3 * ex];
        pEntry[0].setW( 0400000000000 );
    }
    pDAS[033].setW( 0400000000000 );

    BLOCK_ID blockId = dasTrackWordAddress / prepFactor;

    //  Write the block containing the DAS (don't need to write the rest of the directory track).
    result = directDiskIo( pActivity, deviceId, ChannelModule::Command::WRITE, blockId, prepFactor, pBuffer );
    return result;
}


//  loadDirectoryTrackInfoCache()
//
//  Loads one track of the pack's MFD into cache.
//  Part of integrating a fixed pack into the fixed pool.
//  Does not stop the exec on IO error - there are cases where this is not appropriate.
MFDManager::Result
MFDManager::loadDirectoryTrackIntoCache
(
    Activity* const     pActivity,
    PackInfo* const     pPackInfo,
    const DSADDR        firstDSAddr,
    const BLOCK_ID      firstBlockId
)
{
    Result result;

    Word36* pBuffer = new Word36[pPackInfo->m_PrepFactor];
    BLOCK_ID blockIdLimit = firstBlockId + BLOCKS_PER_TRACK( pPackInfo->m_PrepFactor );
    DSADDR dsAddr = firstDSAddr;

    for ( BLOCK_ID ioBlockId = firstBlockId; ioBlockId != blockIdLimit; ++ioBlockId )
    {
        //  Read a block of directory sectors
        result = directDiskIo( pActivity,
                               pPackInfo->m_DeviceId,
                               ChannelModule::Command::READ,
                               ioBlockId,
                               pPackInfo->m_PrepFactor, pBuffer );
        if ( result.m_Status != MFDST_SUCCESSFUL )
            return result;

        //  Stage the sectors into cache
        Word36* pSector = pBuffer;
        for ( INDEX sx = 0; sx < SECTORS_PER_BLOCK( pPackInfo->m_PrepFactor ); ++sx )
        {
            m_DirectoryCache.insert( std::make_pair( dsAddr, new Word36[WORDS_PER_SECTOR] ) );
            Word36* pDest = m_DirectoryCache[dsAddr];
            for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
                pDest[wx] = pSector[wx];
            pSector += WORDS_PER_SECTOR;
            ++dsAddr;
        }
    }

    //  Add an entry to the track map (prepend LDAT)
    TRACK_ID trackId = (firstDSAddr >> 6) & 077777777;
    m_DirectoryTrackIdMap[trackId] = firstBlockId;

    return result;
}


//  loadFileAllocations()
//
//  Loads the file allocations for the file indicated by the given main item sector 0.
//  Call under lock().
//
//  This CANNOT be used to load the MFD allocations, because this function relies on the
//  MFD information already being in place.  The MFD information must be created by
//  initialization or recovery.
//
//  Parameters:
//      pActivity:          pointer to controlling Activity
//      mainItem0Addr:      DSADDR of main item sector 0 for the file cycle
//                              Make SURE this is for a disk file - we don't check.
//      pFAT:               pointer to the FileAllocationTable to be loaded
//
//  Returns:
//      standard Result value, since we will be interacting with the Datastore
MFDManager::Result
MFDManager::loadFileAllocations
(
    Activity* const             pActivity,
    const DSADDR                mainItem0Addr,
    FileAllocationTable* const  pFAT
)
{
    Result result;

    //  Read the file cycle's main item.
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, false, &pMainItem0, &result ) )
        return result;

    //  Walk the file cycle's DAD items to build up the FAT
    DSADDR linkAddress = getLinkAddress( pMainItem0[0] );
    while ( linkAddress != 0 )
    {
        //  Read the first/next DAD sector
        Word36* pDadItem = 0;
        if ( !stageDirectorySector( linkAddress, false, &pDadItem, &result ) )
            return result;

        //  Iterate over the DAD entries in the sector, and build FAT entries
        WORD_ID firstWord = pDadItem[2].getW();     // first word represented by this DAD sector
        WORD_ID limitWord = pDadItem[3].getW();     // last word represented by this DAD sector + 1
        INDEX ex = 0;
        WORD_ID fileWord = firstWord;
        Word36* pEntry = pDadItem + 4 + (3 * ex);
        while ( (ex < 8) && (fileWord < limitWord) )
        {
            LDATINDEX ldatIndex = pEntry[2].getH2();
            if ( ldatIndex != 0400000 )
            {
                //  Allocated space
                TRACK_ID fileTrackId = fileWord / 1792;
                TRACK_ID deviceTrackId = pEntry[0].getW() / 1792;
                TRACK_COUNT trackCount = pEntry[1].getW() / 1792;
                if ( !pFAT->allocated( fileTrackId, trackCount, ldatIndex, deviceTrackId ) )
                {
                    std::stringstream strm;
                    strm << "MFDManager::loadFileAllocations() pFAT->allocated failed ftid=0" << std::oct << fileTrackId
                        << " tracks=0" << std::oct << trackCount
                        << " ldat=0" << std::oct << ldatIndex
                        << " dtid=0" << std::oct << deviceTrackId;
                    SystemLog::write( strm.str() );
                    result.m_Status = MFDST_INTERNAL_ERROR;
                    return result;
                }
            }

            fileWord += pEntry[1].getW();
            ++ex;
            pEntry += 3;
        }

        //  Get the link address of the next DAD sector
        linkAddress = getLinkAddress( pDadItem[0] );
    }

    //  Synchronize the DAD tables in the FAT
    pFAT->synchronizeDADTables();

    //  Done
    return result;
}


//  loadFixedPackAllocationTable()
//
//  Loads the DiskAllocationTable in the PackInfo object for the indicated pack.
MFDManager::Result
MFDManager::loadFixedPackAllocationTable
(
    Activity* const     pActivity,
    CITPACKINFOMAP      itPackInfo
)
{
    Result result;

    //  Get the PackInfo object for this LDAT
    PackInfo* pPackInfo = itPackInfo->second;
    pPackInfo->m_DiskAllocationTable.initialize( pPackInfo->m_TotalTracks );

    //  Stage successive SMBT sectors
    DSADDR smbtAddr = (pPackInfo->m_LDATIndex << 18) | static_cast<COUNT32>(pPackInfo->m_S0S1HMBTPadWords / 28);
    TRACK_ID trackId = 0;
    WORD_COUNT smbtWordsLeft = pPackInfo->m_SMBTWords - 2;
    Word36* pSMBTWord = 0;
    bool firstIteration = true;

    while ( smbtWordsLeft )
    {
        if ( !stageDirectorySector( smbtAddr, false, &pSMBTWord, &result ) )
            return result;

        //  Iterate over the successive words in the SMBT sector.
        //  For the first SMBT sector, skip the first (control) word.
        INDEX wx = 0;
        if ( firstIteration )
        {
            wx = 1;
            ++pSMBTWord;
            firstIteration = false;
        }

        while ( (wx < WORDS_PER_SECTOR) && (smbtWordsLeft > 0) )
        {
            UINT32 allocBits = static_cast<UINT32>(pSMBTWord->getW() >> 4);
            if ( allocBits != 0 )
            {
                TRACK_ID subTrackId = trackId;
                for ( COUNT c = 0; c < 32; ++c )
                {
                    if ( allocBits & 0x80000000 )
                    {
                        if ( !pPackInfo->m_DiskAllocationTable.modifyArea( subTrackId, 1, true ) )
                            m_pExec->stopExec( Exec::SC_DIRECTORY_ERROR );
                    }
                    allocBits <<= 1;
                    ++subTrackId;
                }
            }

            ++pSMBTWord;
            trackId += 32;
            --smbtWordsLeft;
            ++wx;
        }

        ++smbtAddr;
    }

    return result;
}


//  loadFixedPackAllocationTables()
//
//  Iterates over the set of fixed packs
MFDManager::Result
MFDManager::loadFixedPackAllocationTables
(
    Activity* const     pActivity
)
{
    Result result;
    for (CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        if ( itpi->second->m_InFixedPool )
        {
            result = loadFixedPackAllocationTable( pActivity, itpi );
            if ( result.m_Status != MFDST_SUCCESSFUL )
                break;
        }
    }

    return result;
}


//  readDiskLabel()
//
//  Reads the disk label AND sector 1 for the indicated device, populating a newly-acquired PackInfo object.
//  Called when a pack goes online, and during boot.
//  Any IO error in reading VOL1 or Sector1 causes the device to go DN.
//
//  Upon success, returns a pointer to a newly-allocated PackInfo object to ppPackInfo.
MFDManager::Result
MFDManager::readDiskLabel
(
    Activity* const                 pActivity,
    const DeviceManager::DEVICE_ID  deviceId,
    PackInfo** const                ppPackInfo
)
{
    Result result;
    *ppPackInfo = 0;

    //  Get disk geometry.  This tells us how large a buffer we need for reading the label,
    //  and it avoids an issue on first boot where the unit is in UNIT_ATTENTION state.
    BLOCK_COUNT blockCount;
    BLOCK_SIZE blockSize;
    bool isMounted;
    bool isReady;
    bool isWriteProtected;
    result = getDiskDeviceValues( pActivity, deviceId, &blockCount, &blockSize, &isMounted, &isReady, &isWriteProtected );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        if ( !pActivity->isTerminating() )
            m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );
        return result;
    }

    if ( blockSize == 0 )
    {
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId ) + " VOL1 Label Missing";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);
        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId  );
        result.m_Status = MFDST_PACK_NOT_FORMATTED;
        return result;
    }

    //  Allocate a buffer large enough for the largest possible prep factor and a PackInfo object.
    //  For unprepped packs, block size will be zero; we allow for that, in order to get a non-prepped
    //  result instead of a block size error result.
    PREP_FACTOR prepFactor = miscGetPrepFactorFromBlockSize( blockSize );
    Word36* pBuffer = new Word36[prepFactor];
    PackInfo* pPackInfo = new PackInfo();
    pPackInfo->m_IsMounted = true;
    pPackInfo->m_DeviceId = deviceId;

    //  Read disk label (always block 2)
    result = directDiskIo( pActivity, deviceId, ChannelModule::Command::READ, 2, prepFactor, pBuffer );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        if ( result.m_Status != MFDST_TERMINATING )
            m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );
        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Does it have a valid VOL1 label?
    std::string vol1Str = miscWord36AsciiToString( pBuffer, 1, false );
    if ( vol1Str.compare("VOL1") != 0 )
    {
        result.m_Status = MFDST_PACK_NOT_PREPPED;
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId ) + " VOL1 Label Missing";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);

        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId  );

        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Make sure pack name is valid
    pPackInfo->m_PackName = miscWord36AsciiToString( pBuffer + 1, 2, false );
    pPackInfo->m_PackName.trimTrailingSpaces();
    if ( !miscIsValidPackName( pPackInfo->m_PackName ) )
    {
        result.m_Status = MFDST_PACK_NOT_PREPPED;
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId ) + " Pack name is not valid";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);

        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );

        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Make sure the pack isn't insanely large.  For obsolete definitions of insane, of course.
    if ( pPackInfo->m_TotalTracks > 8383936 )
    {
        result.m_Status = MFDST_UNSUPPORTED_PACK;
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId )
                                + " PACKS LARGER THAN 67GB ARE NOT SUPPORTED.";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);

        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );

        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Get address of the first directory track, the prep factor of the pack,
    //  and the number of software tracks (which doesn't include initial track allocation).
    pPackInfo->m_DirectoryTrackAddress = pBuffer[3].getW();
    pPackInfo->m_PrepFactor = pBuffer[4].getH2();
    pPackInfo->m_TotalTracks = pBuffer[016].getW();

    //  Get sector counts
    pPackInfo->m_S0S1HMBTPadWords = pBuffer[011].getH1();
    pPackInfo->m_SMBTWords = pBuffer[011].getH2();

    //  Now we need sector 1 of the directory.
    //  For prepfactor 28, we will read that sector as a block.
    //  For all other prepfactors, it will be the 2nd sector in the block that we read.
    //  So... figure out the block which contains sector 1, and read it in.
    COUNT sectorsPerBlock = SECTORS_PER_BLOCK(pPackInfo->m_PrepFactor);
    BLOCK_ID blockId = pPackInfo->m_DirectoryTrackAddress / sectorsPerBlock;
    if ( pPackInfo->m_PrepFactor == 28 )
        ++blockId;

    result = directDiskIo( pActivity, deviceId, ChannelModule::Command::READ, blockId, pPackInfo->m_PrepFactor, pBuffer );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );
        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Get pointer to sector 1.
    Word36* pSector1 = pBuffer;
    if ( pPackInfo->m_PrepFactor != 28 )
        pSector1 += WORDS_PER_SECTOR;

    //  Make sure the PACKID in Sector1 and the label match
    std::string sector1PackId = miscWord36FieldataToString( pSector1 + 4, 1 );
    if ( pPackInfo->m_PackName.compare( sector1PackId ) != 0 )
    {
        result.m_Status = MFDST_SECTOR1_CONFLICT;
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId )
                                + " PACKID IN SECTOR1 AND VOL1 DO NOT MATCH.";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);

        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );

        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Check Sector1 Version number - it must be 0 or 1.
    if ( pSector1[010].getS3() > 1 )
    {
        result.m_Status = MFDST_SECTOR1_CONFLICT;
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId )
                                + " SECTOR-1 HAS AN INVALID VERSION NUMBER.";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);

        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );

        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Check DAS offset - legal values are 0 or an even multiple of 576
    if ( pSector1[020].getH2() % 576 > 0 )
    {
        result.m_Status = MFDST_SECTOR1_CONFLICT;
        std::string consMsg = "DEVICE " + m_pDeviceManager->getNodeName( deviceId )
                                + " SECTOR-1 HAS AN INVALID DAS OFFSET.";
        m_pConsoleManager->postReadOnlyMessage(consMsg, 0);

        m_pDeviceManager->setNodeDownInternally( pActivity, deviceId );

        delete[] pBuffer;
        delete pPackInfo;
        return result;
    }

    //  Get LDAT index - if it is 0400000 then this is a freshly-prepped fixed pack.
    //  If it is anything else non-zero, then it is non-freshly-prepped fixed.
    //  If it is zero, it is removable.
    pPackInfo->m_LDATIndex = pSector1[5].getH1();
    pPackInfo->m_IsFixed = ( pPackInfo->m_LDATIndex != 0 );

    //  Done - pass the PackInfo object pointer back to the caller.
    *ppPackInfo = pPackInfo;
    delete[] pBuffer;
    return result;
}


//  readDiskLabels()
//
//  This is one of the first things called during the boot.
//  We call readDiskLabel() for all disk devices, to collect PackInfo objects for all readable packs.
//  We do NOT put these into m_PackInfo, because that map is indexed on the pack's LDAT,
//  and we don't (necessarily) know that yet.
MFDManager::Result
MFDManager::readDiskLabels
(
    Activity* const                 pActivity,
    PACKINFOLIST* const             pPackInfoList
)
{
    //  Don't lock - this is done early in the boot process, so it isn't necessary.
    MFDManager::Result result;
    pPackInfoList->clear();

    DeviceManager::NODE_IDS nodeIds;
    m_pDeviceManager->getDeviceIdentifiers( &nodeIds, Device::DeviceType::DISK );
    for ( INDEX nx = 0; nx < nodeIds.size(); ++nx )
    {
        const DeviceManager::DeviceEntry* pDevEntry = m_pDeviceManager->getDeviceEntry( nodeIds[nx] );
        if ( (pDevEntry->m_Status == DeviceManager::NDST_SU) || (pDevEntry->m_Status == DeviceManager::NDST_UP) )
        {
            PackInfo* pPackInfo = 0;
            MFDManager::Result result = readDiskLabel( pActivity, nodeIds[nx], &pPackInfo );
            if ( result.m_Status == MFDST_TERMINATING )
                return result;
            else if ( result.m_Status == MFDST_SUCCESSFUL )
                pPackInfoList->push_back( pPackInfo );
        }
    }

    return result;
}


/*TODO:RECOV
//  recoverLookupTable()
//
//  Recovers the lookup table by reading all the allocated directory sectors, looking for the search items.
//
//  We make things easier for ourselves by guaranteeing (during processing) that there are no completely
//  empty search items, and that the entry(ies) in each search item contents are contiguous and begin in the
//  first position.
MFDManager::Result
    MFDManager::recoverLookupTable()
{
    Result result;

    //  Get allocated directory sectors, and iterate over them.
    std::set<DSADDR> container;
    result = getAllocatedDirectorySectors( &container, false );
    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    for each ( DSADDR dsAddr in container )
    {
        Word36* pSector = m_DataStoreCache.readDirectorySector( dsAddr, &result.m_DataStoreResult );
        if ( !pSector )
        {
            result.m_Status = MFDST_DATASTORE_ERROR;
            return result;
        }

        //  If this is a lead item, and it's the head of it's containing chain,
        //  create an entry in the hash table for it.
        if ( isLeadItem( pSector ) && !isLinkAddress( pSector[27] ) )
        {
            Word36* pQualifier = &pSector[1];
            Word36* pFilename = &pSector[3];
            INDEX hashIndex = getLookupTableHashIndex( pQualifier, pFilename );
            m_SearchItemLookupTable.setValue( hashIndex, dsAddr );
        }
    }

    return result;
}
*/


/*TODO:RECOV
//  recoverMFDFat()
//
//  Builds the FAT for the MFD file, which we need before we do anything else with the MFD.
//  When we're done, m_MFDMainItem0Addr will be 0, and the FAT will be mapped to DSADDR 0.
MFDManager::Result
    MFDManager::recoverMFDFat()
{
    Result result;

    //  Create a FAT entry so the MFD allocations never get unloaded.
    //  Start out with MFDMainItemAddr set to 0 - we'll fix this later.
    //  However, the allocations and assignment records always use 0 for the dsaddr key.
    m_MFDMainItem0Addr = 0;
    COUNT assignCount = 1;
    m_FileAllocationTable.establishEntry( m_MFDMainItem0Addr );

    //  Read all of the DAS sectors.  The first one is in the first directory track, which starts
    //  in datastore (device) block 0.  Each DAS sector contains allocation information for up to
    //  nine directory tracks.  Generally, there shouldn't be any holes in the MFD allocation,
    //  but we can handle it if there are.
    COUNT wordsPerBlock = m_DataStoreCache.getWordsPerBlock();
    COUNT blocksPerTrack = 1792 / wordsPerBlock;
    Word36* pBuffer = new Word36[wordsPerBlock];
    TRACKID dataStoreTrackId = 0;
    TRACKID dirTrackNo = 0;
    bool done = false;
    while ( !done )
    {
        //  Read datastore block containing the first/next DAS
        BLOCKID blockId = dataStoreTrackId * blocksPerTrack;
        result.m_DataStoreResult = m_DataStoreCache.readBlock( blockId, pBuffer );
        if ( result.m_DataStoreResult.m_Status != DataStore::MFDST_SUCCESSFUL )
        {
            delete[] pBuffer;
            result.m_Status = MFDST_DATASTORE_ERROR;
            m_pConsoleManager->postReadOnlyMessage( DataStoreErrorMsg, GenericRouting, m_pExec->getRunInfo() );
            return result;
        }

        //  The DAS exists in the first MFD track, so we know that this track is allocated.
        //  There is a small chance that we could be contiguous in the datastore space, but it is
        //  so unlikely, we don't bother to check.
        if ( !m_FileAllocationTable.appendAllocation( m_MFDMainItem0Addr, dirTrackNo, dataStoreTrackId, 1 ) )
        {
            result.m_Status = MFDST_INTERNAL_ERROR;
            m_pConsoleManager->postReadOnlyMessage( InternalErrorMsg, GenericRouting, m_pExec->getRunInfo() );
            SystemLog::write( "MFDManager::recoverMFDFat():appendAllocation(1) for MFD failed" );
            return result;
        }

        //  Check the other eight entries in the DAS, which may or may not represent allocated MFD tracks.
        INDEX ex = 1;
        ++dirTrackNo;
        Word36* pEntry = pBuffer + 3;
        while ( ex < 9 )
        {
            if ( isLinkAddress( pEntry[0] ) )
            {
                //  The track reference by this entry is allocated.
                dataStoreTrackId = static_cast<TRACKID>( pEntry->getW() / 1792 );
                if ( !m_FileAllocationTable.appendAllocation( m_MFDMainItem0Addr, dirTrackNo, dataStoreTrackId, 1 ) )
                {
                    result.m_Status = MFDST_INTERNAL_ERROR;
                    m_pConsoleManager->postReadOnlyMessage( InternalErrorMsg, GenericRouting, m_pExec->getRunInfo() );
                    SystemLog::write( "MFDManager::recoverMFDFat():appendAllocation(2) for MFD failed" );
                    return result;
                }
            }

            ++ex;
            pEntry += 3;
            ++dirTrackNo;
        }

        //  If there isn't a link, then this is the last DAS - otherwise, get the track id for the next DAS.
        if ( isLinkAddress( pBuffer[0] ) )
            done = true;
        else
            dataStoreTrackId = static_cast<TRACKID>( pBuffer->getW() / 1792 );
    }

    delete[] pBuffer;
    return result;
}
*/


//  removeLookupEntry()
//
//  Removes a lookup entry for the given qual/file combination.
//
//  Parameters:
//      pActivity:          pointer to controlling Activity
//      pQualifier:         pointer to 2-entry Word36 buffer containing LJSF qualfier in fieldata
//      pFileName:          pointer to 2-entry Word36 buffer containing LJSF filename in fieldata
MFDManager::Result
MFDManager::removeLookupEntry
(
    Activity* const         pActivity,
    const Word36* const     pQualifier,
    const Word36* const     pFileName
)
{
    Result result;

    //  Find search item which contains the indicated qual/file.
    //  If it doesn't exist, that's probably a problem, but it's self-correcting...
    INDEX hashIndex = getLookupTableHashIndex( pQualifier, pFileName );
    DSADDR firstSearchItemAddr = m_SearchItemLookupTable[hashIndex];
    if ( firstSearchItemAddr != 0 )
    {
        //  We found a current lookup entry at the given hash index - walk the chain
        DSADDR searchItemAddr = firstSearchItemAddr;
        while ( searchItemAddr != 0 )
        {
            //  Grab the search item sector corresponding to searchItemAddr.
            Word36* pSearchItem = 0;
            if ( !stageDirectorySector( searchItemAddr, false, &pSearchItem, &result ) )
                return result;

            //  Look for the indicated qual/file
            Word36* pEntry = &pSearchItem[1];
            for ( INDEX ex = 0; ex < 5; ++ex )
            {
                if ( (pEntry[0].getW() == pQualifier[0].getW())
                    && (pEntry[1].getW() == pQualifier[1].getW())
                    && (pEntry[2].getW() == pFileName[0].getW())
                    && (pEntry[3].getW() == pFileName[1].getW()) )
                {
                    //  Found it - zero it out.
                    pEntry[0].setW( 0 );
                    pEntry[1].setW( 0 );
                    pEntry[2].setW( 0 );
                    pEntry[3].setW( 0 );
                    pEntry[4].setW( 0 );

                    DEBUG_INSERT( searchItemAddr );
                    return result;
                }

                pEntry += 5;
            }

            searchItemAddr = getLinkAddress( pSearchItem[0] );
        }
    }
    else
    {
        std::stringstream strm;
        strm << "MFDManager::removeLookupEntry() DSADDR for hashIndex is zero - hshx=0" << std::oct << hashIndex;
        SystemLog::write( strm.str() );
        m_pConsoleManager->postReadOnlyMessage( strm.str(), 0 );
        result.m_Status = MFDST_INTERNAL_ERROR;
    }

    return result;
}


//  setSMBTAllocated()
//
//  Updates the SMBT for a given pack to allocate or deallocate the indicated range of tracks.
//  Does NOT commit the MFD cache - caller must do that separately.
//
//  Parameters:
//      pActivity:          pointer to controlling Activity
//      ldatIndex:          identifies the pack of interest
//      trackId:            device-relative track id of first track in region to be allocated or released
//      trackCount:         number of contiguous tracks to be allocated or released
//      allocated:          true to allocate the region, false to release it
//      updateHMBT:         true to set the bit in the HMBT as well (which we do for bad tracking)
MFDManager::Result
MFDManager::setSMBTAllocated
(
    Activity* const         pActivity,
    const LDATINDEX         ldatIndex,
    const TRACK_ID          trackId,
    const TRACK_COUNT       trackCount,
    const bool              allocated,
    const bool              updateHMBT
)
{
    Result result;

    ITPACKINFOMAP itpi = m_PackInfo.find( ldatIndex );
    if ( itpi == m_PackInfo.end() )
    {
        std::stringstream strm;
        strm << "MFDManager::setSMBTAllocated() Cannot find pack for ldatIndex=0" << std::oct << ldatIndex;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    PackInfo* pPackInfo = itpi->second;
    if ( !pPackInfo->m_IsMounted )
    {
        std::stringstream strm;
        strm << "MFDManager::setSMBTAllocated() Pack is not mounted for ldatIndex=0" << std::oct << ldatIndex;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    if ( trackId + trackCount >= pPackInfo->m_TotalTracks )
    {
        std::stringstream strm;
        strm << "MFDManager::setSMBTAllocated() trackId=0" << std::oct << trackId
            << " + trackCount=0" << std::oct << trackCount
            << " > totalTracks=0" << std::oct << pPackInfo->m_TotalTracks
            << " for ldatIndex=0" << std::oct << ldatIndex;
        SystemLog::write( strm.str() );
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    //  Get starting offsets, IDs, and addresses for the first track in question
    COUNT32 bitOffset = trackId % 32;                       //  a particular bit in the associated MBT word
    UINT64 bitMask = 0400000000000LL >> bitOffset;          //  mask with associated bit set, all else clear
    WORD_COUNT mbtWordOffset = (trackId / 32) + 1;          //  Word offset of associated MBT word, from start of MBT
    WORD_COUNT wordOffset = mbtWordOffset % 28;             //  Offset of SMBT word from start of containing SMBT sector
    DSADDR hmbtAddr = (ldatIndex << 18) | 2;                //  DSADDR of the associated HMBT sector
    DSADDR smbtAddr = (ldatIndex << 18) | static_cast<DSADDR>(pPackInfo->m_S0S1HMBTPadWords / 28);
                                                            //  DSADDR of the associated SMBT sector

    //  Now iterate over the affected tracks.  Update offsets, IDs, and addresses as appropriate, per iteration.
    //  This implementation is far from efficient.  But it's easy, and thus more reliable.  In theory.
    TRACK_COUNT iterations = trackCount;
    bool newSector = true;
    Word36* pHMBTSector = 0;
    Word36* pSMBTSector = 0;
    while ( iterations )
    {
        if ( newSector )
        {
            if ( updateHMBT )
            {
                if ( !stageDirectorySector( hmbtAddr, true, &pHMBTSector, &result ) )
                    return result;
            }

            if ( !stageDirectorySector( smbtAddr, true, &pSMBTSector, &result ) )
                return result;
            newSector = false;
        }

        //  Maybe update HMBT word
        if ( updateHMBT )
        {
            if ( allocated )
                pHMBTSector[wordOffset].logicalOr( bitMask );
            else if ( (pHMBTSector[wordOffset].getW() & bitMask) != 0 )
                pHMBTSector[wordOffset].setW( pHMBTSector[wordOffset].getW() ^ bitMask );
        }

        //  Definitely update SMBT word
        if ( allocated )
            pSMBTSector[wordOffset].logicalOr( bitMask );
        else if ( (pSMBTSector[wordOffset].getW() & bitMask) != 0 )
            pSMBTSector[wordOffset].setW( pSMBTSector[wordOffset].getW() ^ bitMask );

        --iterations;
        if ( iterations )
        {
            ++bitOffset;
            bitMask >>= 1;
            if ( bitOffset == 32 )
            {
                bitOffset = 0;
                bitMask = 0400000000000LL;
                ++wordOffset;
                if ( wordOffset == 28 )
                {
                    wordOffset = 0;
                    ++hmbtAddr;
                    ++smbtAddr;
                    newSector = true;
                }
            }
        }
    }

    return result;
}


//  stageDirectorySector
//
//  Convenient wrapper around a common directory cache operation.
//  Search the directory cache for the entry indicated by the given DSADDR - when found,
//  store the pointer to the cached sector in ppSector.
//
//  We presume the directory cache is completely loaded; therefore, not finding the cached sector
//  for the given address is considered an internal error; in this case, we update the given
//  Result object and return false
//
//  This version is for const functions, and never sets a sector updated.
inline bool
MFDManager::stageDirectorySector
(
    const DSADDR    directorySectorAddress,
    Word36** const  ppSector,
    Result* const   pResult
) const
{
    CITDIRECTORYCACHE itSector = m_DirectoryCache.find( directorySectorAddress );
    if ( itSector == m_DirectoryCache.end() )
    {
        std::stringstream strm;
        strm << "MFDManager::stageDirectorySector() const dsAddr=0" << std::oct << directorySectorAddress
            << " is not in cache";
        SystemLog::write( strm.str() );
        pResult->m_Status = MFDST_INTERNAL_ERROR;
        return false;
    }

    *ppSector = itSector->second;

    return true;
}


//  stageDirectorySector
//
//  Convenient wrapper around a common directory cache operation.
//  Search the directory cache for the entry indicated by the given DSADDR - when found,
//  store the pointer to the cached sector in ppSector.
//  If setUpdated is true, we additionally add the sector address to the updated cache set.
//
//  We presume the directory cache is completely loaded; therefore, not finding the cached sector
//  for the given address is considered an internal error; in this case, we update the given
//  Result object and return false
inline bool
MFDManager::stageDirectorySector
(
    const DSADDR    directorySectorAddress,
    const bool      setUpdated,
    Word36** const  ppSector,
    Result* const   pResult
)
{
    bool retn = stageDirectorySector( directorySectorAddress, ppSector, pResult );
    if ( retn && setUpdated )
        DEBUG_INSERT( directorySectorAddress );
    return retn;
}


//  stageLeadItems()
//
//  Ensures the lead item sector 0 (and sector 1 if it exists) are in cache for the indicated file.
//  This version is for const functions which are not going to update the lead items.
//
//  Parameters:
//      leadItem0Addr:      DSADDR of lead item sector 0 to be staged
//      pLeadItem1Addr:     where we store the link address for sector 1 (if it exists), else we store 0
//      ppSector0:          where we store a pointer to the staged lead item sector 0
//      ppSector1:          where we store a pointer to the staged lead item sector 1 if it exists, else we store 0
//      pResult:            where we store status if non SUCCESSFUL
inline bool
MFDManager::stageLeadItems
(
    const DSADDR            leadItem0Addr,
    DSADDR* const           pLeadItem1Addr,
    Word36** const          ppSector0,
    Word36** const          ppSector1,
    Result* const           pResult
) const
{
    if ( !stageDirectorySector( leadItem0Addr, ppSector0, pResult ) )
        return false;

    *ppSector1 = 0;
    *pLeadItem1Addr = getLinkAddress( (*ppSector0)[0] );
    if ( *pLeadItem1Addr != 0 )
    {
        if ( !stageDirectorySector( *pLeadItem1Addr, ppSector1, pResult ) )
            return false;
    }

    return true;
}


//  stageLeadItems()
//
//  Ensures the lead item sector 0 (and sector 1 if it exists) are in cache for the indicated file.
//
//  Parameters:
//      leadItem0Addr:      DSADDR of lead item sector 0 to be staged
//      updateFlag:         true if we are going to update either or both of the sectors
//      pLeadItem1Addr:     where we store the link address for sector 1 (if it exists), else we store 0
//      ppSector0:          where we store a pointer to the staged lead item sector 0
//      ppSector1:          where we store a pointer to the staged lead item sector 1 if it exists, else we store 0
//      pResult:            where we store status if non SUCCESSFUL
inline bool
MFDManager::stageLeadItems
(
    const DSADDR            leadItem0Addr,
    const bool              updateFlag,
    DSADDR* const           pLeadItem1Addr,
    Word36** const          ppSector0,
    Word36** const          ppSector1,
    Result* const           pResult
)
{
    bool retn = stageLeadItems( leadItem0Addr, pLeadItem1Addr, ppSector0, ppSector1, pResult );
    if ( retn && updateFlag )
    {
        DEBUG_INSERT( leadItem0Addr );
        if ( *ppSector1 )
            DEBUG_INSERT( *pLeadItem1Addr );
    }

    return retn;
}


//  stageMainItems()
//
//  Ensures the main item sector 0 (and sector 1 if it exists) are in cache for the indicated file.
//
//  Parameters:
//      mainItem0Addr:      DSADDR of main item sector 0 to be staged
//      updateFlag:         true if we are going to update either or both of the sectors
//      pMainItem1Addr:     where we store the link address for sector 1 (if it exists), else we store 0
//      ppSector0:          where we store a pointer to the staged main item sector 0
//      ppSector1:          where we store a pointer to the staged main item sector 1 if it exists, else we store 0
//      pResult:            where we store status if non SUCCESSFUL
inline bool
MFDManager::stageMainItems
(
    const DSADDR            mainItem0Addr,
    const bool              updateFlag,
    DSADDR* const           pMainItem1Addr,
    Word36** const          ppSector0,
    Word36** const          ppSector1,
    Result* const           pResult
)
{
    if ( !stageDirectorySector( mainItem0Addr, updateFlag, ppSector0, pResult ) )
        return false;

    *ppSector1 = 0;
    *pMainItem1Addr = getLinkAddress( (*ppSector0)[015] );
    if ( *pMainItem1Addr != 0 )
    {
        if ( !stageDirectorySector( *pMainItem1Addr, updateFlag, ppSector1, pResult ) )
            return false;
    }

    return true;
}


//  stopExecOnResultStatus()
//
//  Check given result status which is going to be sent back to the client...
//  Certain values demand an exec stop.
//
//  Parameters:
//      result:             value to be tested
//      allowIoError:       if true, we do NOT stop the EXEC on IO error
void
MFDManager::stopExecOnResultStatus
(
    const Result&           result,
    const bool              allowIoError
) const
{
    bool logFlag = false;
    if ( result.m_Status == MFDST_INTERNAL_ERROR )
    {
        m_pExec->stopExec( Exec::SC_DIRECTORY_ERROR );
        logFlag = true;
    }
    else if ( !allowIoError && (result.m_Status == MFDST_IO_ERROR) )
    {
        m_pExec->stopExec( Exec::SC_EXEC_IO_FAILED );
        logFlag = true;
    }

    if ( logFlag )
    {
        std::string logMsg = "MFDManager::stopExecOnResultStatus:result=";
        logMsg += getResultString( result );
        SystemLog::write( logMsg );
    }
}


//  updateGranulesInfo()
//
//  Updates the indicated file's main item 0 to report the granule counts
MFDManager::Result
MFDManager::updateGranulesInfo
(
    Activity* const         pActivity,
    const DSADDR            mainItem0Addr,
    const UINT32            initialGranules,
    const UINT32            maximumGranules,
    const UINT32            highestGranuleAssigned,
    const UINT32            highestTrackWritten
)
{
    Result result;

    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
        return result;

    pMainItem0[024].setH1( initialGranules );
    pMainItem0[025].setH1( maximumGranules );
    pMainItem0[026].setH1( highestGranuleAssigned );
    pMainItem0[027].setH1( highestTrackWritten );

    return result;
}


//  updateGuardedBit()
//
//  Checks all the main items linked to the given lead item(s)...
//  If any of them are guarded, then the guard bit is set in lead item 0.
//  Otherwise, the bit is cleared.  (guarded bit is word 012,T1,Bit0)
//  Caller must ensure that the cache manager knows lead item sector 0 has been updated.
//
//  Parameters:
//      pLeadItem0:         pointer to lead item sector 0
//      pLeadItem1:         pointer to lead item sector 1 (if it exists) or 0
//                              Caller must add DSADDR for lead item to updated cache container
//  Returns:
//      Result value (only non-successful if datastore read fails)
MFDManager::Result
MFDManager::updateGuardedBit
(
    Activity* const         pActivity,
    const DSADDR            leadItem0Addr,
    Word36* const           pLeadItem0,
    const Word36* const     pLeadItem1
)
{
    Result result;

    //  Clear the guarded bit
    pLeadItem0[012].logicalAnd( 0377777777777ll );
    DEBUG_INSERT( leadItem0Addr );

    //  Now iterate over the links, and set the guarded bit if we find a guarded main item
    INDEX linkLimit = ( pLeadItem1 == 0 ) ? 17 : 32;
    for ( INDEX ex = 0; ex < linkLimit; ++ex )
    {
        const Word36* pEntry = getMainItemLinkPointer( ex, pLeadItem0, pLeadItem1 );
        if ( !pEntry->isZero() )
        {
            DSADDR mainItem0Addr = getLinkAddress( *pEntry );
            Word36* pMainItem0 = 0;
            if ( !stageDirectorySector( mainItem0Addr, false, &pMainItem0, &result ) )
                return result;

            UINT8 inhibitFlags = pMainItem0[021].getS2();
            if ( inhibitFlags & 040 )
            {
                //  This main item is guarded, so we need to set the flag for the lead item.
                pLeadItem0[012].logicalOr( 0400000000000ll );
                break;
            }
        }
    }

    return result;
}


//  writeDADUpdates()
//
//  Goes through a FileAllocationTable, and writes updated DADs to the MFD.
//TODO:REM currently only works for fixed - needs updated (expanded?) for removable
MFDManager::Result
MFDManager::writeDADUpdates
(
    Activity* const             pActivity,
    FileAllocationTable* const  pFileAllocationTable
)
{
    Result result;

    //  Go through the DADs looking for new ones which need allocations.
    //  As we get allocations, update the neighboring DADs appropriately.
    FileAllocationTable::DADTABLES& dadTables = pFileAllocationTable->getDADTables();
    DSADDR mainItem0Addr = pFileAllocationTable->getMainItem0Addr();
    LDATINDEX preferredLDAT = mainItem0Addr >> 18;

    for ( FileAllocationTable::ITDADTABLES itdt = dadTables.begin(); itdt != dadTables.end(); ++itdt )
    {
        //  If the DAD is updated and has no DSADDR, it is new and needs an allocation
        if ( (*itdt)->isUpdated() && ((*itdt)->getDSAddress() == 0) )
        {
            DSADDR dsAddress = 0;
            result = allocateDirectorySector( pActivity, preferredLDAT, &dsAddress );
            if ( result.m_Status != MFDST_SUCCESSFUL )
                return result;

            (*itdt)->setDSAddress( dsAddress );

            //  If there's a previous DAD, update it - otherwise, update the main item.
            //  We fully expect all new DADs to be at the end of the chain, but we do not assume it.
            if ( itdt != dadTables.begin() )
            {
                //  Find previous item, and splice this item in between that previous item and
                //  its former next item (shouldn't be one, but just in case there is...).
                FileAllocationTable::ITDADTABLES itPrev = itdt;
                --itPrev;
                (*itdt)->setNextDADSector( (*itPrev)->getNextDADSector() );
                (*itdt)->setPreviousDADSector( (*itPrev)->getDSAddress() );
                (*itPrev)->setNextDADSector( dsAddress );
                (*itPrev)->setIsUpdated( true );
            }
            else
            {
                //  Stage main item sector 0, and splice the new DAD between the main item sector 0
                //  and the former first DAD sector.  Again, there shouldn't be a former first DAD
                //  sector, but just in case there is...
                Word36* pMainItem0 = 0;
                if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
                    return result;
                (*itdt)->setNextDADSector( getLinkAddress( pMainItem0[0] ) );
                (*itdt)->setPreviousDADSector( mainItem0Addr );
                pMainItem0[0].setW( 0200000000000l | dsAddress );
            }

            //  If there's a next DAD...
            //  Some of this is redundant.  That's okay.
            FileAllocationTable::ITDADTABLES itNext = itdt;
            ++itNext;
            if ( itNext != dadTables.end() )
            {
                (*itdt)->setNextDADSector( (*itNext)->getDSAddress() );
                (*itdt)->setPreviousDADSector( (*itNext)->getPreviousDADSector() );
                (*itNext)->setPreviousDADSector( dsAddress );
                (*itNext)->setIsUpdated( true );
            }
        }

        //  If the DAD is updated and set for deletion, unlink it from its neighbors.
        //  Do not deallocate it yet.
        if ( (*itdt)->isUpdated() && (*itdt)->isDeleted() )
        {
            if ( itdt != dadTables.begin() )
            {
                FileAllocationTable::ITDADTABLES itPrev = itdt;
                --itPrev;
                (*itPrev)->setNextDADSector( (*itdt)->getNextDADSector() );
                (*itPrev)->setIsUpdated( true );
            }
            else
            {
                Word36* pMainItem0 = 0;
                if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
                    return result;
                pMainItem0[0].setW( 0200000000000l | (*itdt)->getNextDADSector() );
                (*itdt)->setPreviousDADSector( 0 );
            }

            FileAllocationTable::ITDADTABLES itNext = itdt;
            ++itNext;
            if ( itNext != dadTables.end() )
            {
                (*itNext)->setPreviousDADSector( (*itdt)->getPreviousDADSector() );
                (*itNext)->setIsUpdated( true );
            }

            (*itdt)->setPreviousDADSector( 0 );
            (*itdt)->setNextDADSector( 0 );
        }
    }

    //  Now write all the updated (but not deleted) DAD tables to their corresponding MFD sectors.
    for ( FileAllocationTable::ITDADTABLES itdt = dadTables.begin(); itdt != dadTables.end(); ++itdt )
    {
        if ( (*itdt)->isUpdated() && !(*itdt)->isDeleted() )
        {
            const Word36* pDAD = (*itdt)->getData();
            Word36* pDADSector = 0;
            if ( !stageDirectorySector( (*itdt)->getDSAddress(), true, &pDADSector, &result ) )
                return result;

            for ( INDEX wx = 0; wx < 28; ++wx )
                pDADSector[wx] = pDAD[wx];
            (*itdt)->setIsUpdated( false );
        }
    }

    //  Now go back through the DAD tables once more, and deallocate any DADs marked for deletion.
    //  Also, remove them from the container.
    FileAllocationTable::ITDADTABLES itdt = dadTables.begin();
    while ( itdt != dadTables.end() )
    {
        if ( (*itdt)->isUpdated() && (*itdt)->isDeleted() )
        {
            result = deallocateDirectorySector( pActivity, (*itdt)->getDSAddress() );
            itdt = dadTables.erase( itdt );
        }
        else
            ++itdt;
    }

    //  All done.
    return result;
}


//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------
//  private statics
//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------

//  buildEmptyDASSector()
//
//  Builds up an empty DAS sector in the given buffer.
void
MFDManager::buildEmptyDASSector
(
    Word36* const       pDAS,
    const LDATINDEX     ldatIndex
)
{
    pDAS[0].setH1( ldatIndex );
    pDAS[0].setH2( 0 );

    //  Only the first sector (the DAS sector) is used - set the flag for it.
    pDAS[1].setW( 0400000000000 );
    pDAS[2].setW( 0 );
    for ( INDEX ex = 1; ex < 9; ++ex )
    {
        Word36* pEntry = &pDAS[3 * ex];
        pEntry[0].setW( 0400000000000 );
        pEntry[1].setW( 0 );
        pEntry[2].setW( 0 );
    }

    pDAS[033].setW(0400000000000);
}


//  buildSector0ForLargeDisks()
//
//  Fills in a Sector0 struct for disks which require 9 or more directory tracks
void
MFDManager::buildSector0ForLargeDisks
(
    Word36* const       pSector0,
    const SECTOR_ID     firstDirectoryTrackAddress,
    const TRACK_COUNT   directoryTracks
)
{
    //  Large disk format for sector 0 (no DAS in sector 0)
    //  There is (or will be) a non-zero DAS offset in sector 1, 020:H2
    pSector0[0].setS1( directoryTracks );
    DRWA firstDASAddr = (firstDirectoryTrackAddress * WORDS_PER_SECTOR) + (directoryTracks * WORDS_PER_TRACK);
    pSector0[033].setW( firstDASAddr );
}


//  buildSector0ForSmallDisks()
//
//  Fills in a Sector0 struct for disks which require fewer than 9 directory tracks
void
MFDManager::buildSector0ForSmallDisks
(
    Word36* const       pSector0,
    const SECTOR_ID     firstDirectoryTrackAddress,
    const WORD_COUNT    hmbtLength,
    const WORD_COUNT    smbtLength
)
{
    //  Small disk format for sector 0 (DAS sector).
    //  There are less than 9 directory tracks, so we will only need one DAS.
    //  Sector 1, 020:H2 must be zero.
    DRWA directoryTrackAddress = firstDirectoryTrackAddress;
    COUNT sectors = static_cast<COUNT>((hmbtLength + smbtLength) / 28);
    if ( (hmbtLength + smbtLength) % 28 )
        ++sectors;
    for ( INDEX tx = 0; tx < 9; ++tx )
    {
        Word36* pEntry = &pSector0[3 * tx];
        if ( sectors == 0 )
            pEntry[0].setW(0400000000000);
        else
        {
            pEntry[0].setW(directoryTrackAddress++);
            pEntry[1].setW(0777777777760ll);
            if ( sectors < 32 )
            {
                pEntry[1].leftShiftLogical(32 - sectors);
                sectors = 0;
            }
            else
            {
                sectors -= 32;
                pEntry[2].setW(0777777777760ll);
                if ( sectors < 32 )
                {
                    pEntry[2].leftShiftLogical(32 - sectors);
                    sectors = 0;
                }
                else
                    sectors -= 32;
            }
        }
    }
}


//  calculateCycleInfo()
//  Inspects the main item links in lead item 0 and the optional lead item 1, and updates:
//      current cycle range in lead item 0 (word 011, Q3)
//      current number of cycles (not counting those in to-be state) (word 011, S2)
//  Caller must ensure that sector 0 has been staged for update.
//
//  Parameters:
//      pLeadItem0:     pointer to lead item 0
//      pLeadItem1:     pointer to lead item 1 if it exists, else 0
void
MFDManager::calculateCycleInfo
(
    Word36* const       pLeadItem0,
    const Word36* const pLeadItem1
)
{
    //  Find the index of the first (highest) entry.
    //  Normally (but not necessarily always) this will be entry 0.
    //  If there are no valid links (which could happen in the middle of some operation),
    //  we will set the current range and count to zero.
    INDEX firstEntry = 0;
    COUNT maxLinks = (pLeadItem1 == 0) ? 17 : 32;
    while ( firstEntry < maxLinks )
    {
        const Word36* const pEntry = getMainItemLinkPointer( firstEntry, pLeadItem0, pLeadItem1 );
        if ( !pEntry->isZero() )
            break;
        ++firstEntry;
    }

    if ( firstEntry == maxLinks )
    {
        pLeadItem0[011].setS2( 0 );
        pLeadItem0[011].setQ3( 0 );
        return;
    }

    //  Keep iterating over the links, to find the last valid one.
    COUNT cycles = 1;
    INDEX lastEntry = firstEntry;
    INDEX thisEntry = firstEntry;
    while ( thisEntry < maxLinks )
    {
        const Word36* const pEntry = getMainItemLinkPointer( firstEntry, pLeadItem0, pLeadItem1 );
        if ( !pEntry->isZero() )
        {
            lastEntry = thisEntry;
            if ( !pEntry->isNegative() )
                ++cycles;
        }
        ++thisEntry;
    }

    pLeadItem0[011].setS2( cycles );
    pLeadItem0[011].setQ3( lastEntry - thisEntry + 1 );
}


//  dumpMFDTrack()
//
//  Dumps a particular MFD track provided to us
void
MFDManager::dumpMFDTrack
(
    std::ostream&       stream,
    const TRACK_ID      directoryTrackId,
    const LDATINDEX     ldatIndex,
    const TRACK_ID      deviceTrackId,
    const Word36* const pData,
    const UINT64        allocMask
)
{
    stream << "      MFDTrackId=0" << std::oct << directoryTrackId
        << " LDATIndex=0" << std::oct << ldatIndex
        << " DeviceTrackId=0" << std::oct << deviceTrackId
        << std::endl;

    UINT64 workingMask = 0x8000000000000000;
    const Word36* pSector = pData;
    for ( INDEX sx = 0; sx < 64; ++sx )
    {
        stream << "      Sector 0" << std::oct << sx;
        if ( (allocMask & workingMask) != 0 )
        {
            stream << std::endl;
            for ( INDEX ix = 0; ix < 4; ++ix )
            {
                stream << "        ";

                for ( INDEX iy = 0; iy < 7; ++iy )
                    stream << std::oct << std::setw( 12 ) << std::setfill( '0' ) << pSector[ix * 7 + iy] << " ";

                stream << " ";
                for ( INDEX iy = 0; iy < 7; ++iy )
                    stream << miscWord36FieldataToString( &pSector[ix * 7 + iy], 1 ) << " ";

                for ( INDEX iy = 0; iy < 7; ++iy )
                    stream << miscWord36AsciiToString( &pSector[ix * 7 + iy], 1, true, '.' ) << " ";

                stream << std::endl;
            }
        }
        else
            stream << " unallocated" << std::endl;

        workingMask >>= 1;
        pSector += 28;
    }
}


//  getCommonFileCycleInfo()
//
//  Common code for getMassStorageFileCycleInfo() and getTapeFileCycleInfo()
void
    MFDManager::getCommonFileCycleInfo
    (
    FileCycleInfo* const    pInfo,
    const DSADDR            mainItem0Addr,
    const Word36* const     pMainItem0,
    const DSADDR            mainItem1Addr,
    const Word36* const     pMainItem1
    )
{
    UINT8 disables = pMainItem0[013].getS1();
    UINT16 descriptors = pMainItem0[014].getT1();
    UINT8 inhibits = pMainItem0[021].getS2();

    pInfo->m_DirectorySectorAddresses.resize( 2 );
    pInfo->m_DirectorySectorAddresses[0] = mainItem0Addr;
    pInfo->m_DirectorySectorAddresses[1] = mainItem1Addr;

    pInfo->m_Qualifier = miscWord36FieldataToString( &pMainItem0[01], 2 );
    pInfo->m_FileName = miscWord36FieldataToString( &pMainItem0[03], 2 );
    pInfo->m_ProjectId = miscWord36FieldataToString( &pMainItem0[05], 2 );
    pInfo->m_AccountId = miscWord36FieldataToString( &pMainItem0[07], 2 );
    pInfo->m_AssignMnemonic = miscWord36FieldataToString( &pMainItem0[016], 1 );
    pInfo->m_AbsoluteCycle = pMainItem0[021].getT3();
    pInfo->m_ToBeCataloged = (descriptors & 0100) ? true : false;
    pInfo->m_ToBeDropped = (descriptors & 0001) ? true : false;
    pInfo->m_ToBeReadOnly = (descriptors & 0002) ? true : false;
    pInfo->m_ToBeWriteOnly = (descriptors & 0004) ? true : false;
    pInfo->m_DirectoryDisabled = (disables & 020) ? true : false;
    pInfo->m_WrittenToDisabled = (disables & 010) ? true : false;
    pInfo->m_InaccessibleBackupDisabled = (disables & 004) ? true : false;
    pInfo->m_BackedUp = (descriptors & 02000) ? true : false;
    pInfo->m_Guarded = (inhibits & 040) ? true : false;
    pInfo->m_Private = (inhibits & 010) ? true : false;
    pInfo->m_ExclusiveUse = (inhibits & 004) ? true : false;
    pInfo->m_ReadInhibited = (inhibits & 002) ? true : false;
    pInfo->m_WriteInhibited = (inhibits & 001) ? true : false;
    pInfo->m_Queued = pMainItem0[017].getH1() == 0 ? false : true;
    pInfo->m_CumulativeAssignCount = pMainItem0[017].getH2();
    pInfo->m_CurrentAssignCount = pMainItem0[021].getT2();
    pInfo->m_TDateCurrentAssignment = pMainItem0[022];
    pInfo->m_TDateCataloged = pMainItem0[023];

    if ( !pMainItem1[010].isZero() )
    {
        UINT8 fasBits = pMainItem1[011].getS2();
        pInfo->m_BackupInfo.m_TDateBackup = pMainItem1[010];
        pInfo->m_BackupInfo.m_UnloadedAtBackup = ( fasBits & 040 ) ? true : false;
        pInfo->m_BackupInfo.m_MaxBackupLevels = pMainItem1[011].getS1();
        pInfo->m_BackupInfo.m_CurrentBackupLevels = pMainItem1[011].getS3();
        pInfo->m_BackupInfo.m_NumberOfBlocks = pMainItem1[011].getH2();
        bool blocksAsPositions = ( fasBits & 020 ) ? true : false;
        if ( blocksAsPositions )
            pInfo->m_BackupInfo.m_NumberOfBlocks *= 64;
        pInfo->m_BackupInfo.m_StartingFilePosition = static_cast<COUNT32>(pMainItem1[012].getW());
        if ( !pMainItem1[013].isZero() )
            pInfo->m_BackupInfo.m_BackupReels.push_back( miscWord36FieldataToString( &pMainItem1[013], 1 ) );
        if ( !pMainItem1[014].isZero() )
            pInfo->m_BackupInfo.m_BackupReels.push_back( miscWord36FieldataToString( &pMainItem1[014], 1 ) );
    }

    switch ( pMainItem1[017].getS2() )
    {
    case 1:
        pInfo->m_TipInfo = TIPINFO_DUPLEX_LEG_1;
        break;
    case 2:
        pInfo->m_TipInfo = TIPINFO_DUPLEX_LEG_2;
        break;
    case 3:
        pInfo->m_TipInfo = TIPINFO_SIMPLEX;
        break;
    default:
        pInfo->m_TipInfo = TIPINFO_NON_TIP;
        break;
    }
}


//  getDiskDeviceInfo()
//
//  Issues an INQUIRY to the given device, and produces a pointer to a dyanmically-allocated
//  DeviceManager::DiskDeviceInfo36 object if successful, 0 if not successful.
MFDManager::Result
MFDManager::getDiskDeviceInfo
(
    Activity* const                         pActivity,
    const DeviceManager::DEVICE_ID          deviceId,
    DeviceManager::DiskDeviceInfo36** const ppInfo
)
{
    Word36 buffer[28];
    Result result = directDiskIo( pActivity, deviceId, ChannelModule::Command::INQUIRY, 0, 28, buffer );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        *ppInfo = 0;
        return result;
    }

    //  There is some trickery needed here, as the raw device data is in 8-bit mode,
    //  and we're living in 36-bit mode -- directDiskIo() always uses xlat mode C, which packs
    //  9 bytes into 2 words.  So... we have to deal with that.
    *ppInfo = DeviceManager::DiskDeviceInfo36::createFromBuffer( buffer );
    return result;
}


//  getDiskDeviceValues()
//
//  Employs getDiskDeviceInfo() to pull out a struct, then pulls individual values from that struct
//  Useful mainly in that it avoids requiring the user to care about more than he does,
//  or to worry about releasing the info object.
MFDManager::Result
MFDManager::getDiskDeviceValues
(
    Activity* const                 pActivity,
    const DeviceManager::DEVICE_ID  deviceId,
    BLOCK_COUNT* const              pBlockCount,
    BLOCK_SIZE* const               pBlockSize,
    bool* const                     pIsMounted,
    bool* const                     pIsReady,
    bool* const                     pIsWriteProtected
)
{
    Result result;
    DeviceManager::DiskDeviceInfo36* pInfo;
    result = getDiskDeviceInfo( pActivity, deviceId, &pInfo );
    if ( result.m_Status == MFDST_SUCCESSFUL )
    {
        if ( pBlockCount )
            *pBlockCount = pInfo->m_BlockCount;
        if ( pBlockSize )
            *pBlockSize = pInfo->m_BlockSize;
        if ( pIsMounted )
            *pIsMounted = pInfo->m_IsMounted;
        if ( pIsReady )
            *pIsReady = pInfo->m_IsReady;
        if ( pIsWriteProtected )
            *pIsWriteProtected = pInfo->m_IsWriteProtected;

        delete pInfo;
        pInfo = 0;
    }

    return result;
}


//  getMBTOffsetAndMask()
//
//  Converts a track offset from the beginning of a device, to the offset into the device's MBT
//  of the MBT word which contains the bit which represents that track, and a bit mask for the
//  word with a single bit set in the position which represents the track.
inline void
MFDManager::getMBTOffsetAndMask
(
    const TRACK_ID      trackId,
    COUNT32* const      pWordOffset,
    UINT64* const       pBitMask
)
{
    COUNT32 bitOffset = trackId % 32;
    *pWordOffset = static_cast<COUNT32>((trackId / 32) + 1);
    *pBitMask = 0400000000000ll >> bitOffset;
}


//  InternalPrepBuildMBT()
//
//  Builds an MBT in memory.  We assume the given buffer is pre-set to zeros.
//  subfunction for internalPrep()
void
MFDManager::internalPrepBuildMBT
(
    Word36* const       pMasterBitTable,
    const TRACK_COUNT   totalTracks,
    const TRACK_COUNT   reservedTracks,
    const TRACK_COUNT   directoryTracks,
    const SECTOR_ID     firstDirectoryTrackSectorAddr,
    const WORD_COUNT    mbtWords
)
{
    //  Crow-bar the reserved track bits into place.  (They're always the first {n} tracks)
    switch ( reservedTracks )
    {
    case 1:
        pMasterBitTable[1].setW(0400000000000);
        break;

    case 2:
        pMasterBitTable[1].setW(0600000000000);
        break;

    case 3:
        pMasterBitTable[1].setW(0700000000000);
        break;
    }

    //  Now take care of all of the intial reserve tracks.
    //  This is directory tracks, plus another track if directoryTracks >= 9;
    TRACK_ID directoryTrack = firstDirectoryTrackSectorAddr / 64;
    TRACK_COUNT tracks = directoryTracks + ((directoryTracks >= 9) ? 1 : 0);
    while ( tracks )
    {
        COUNT32 wordOffset;
        UINT64 bitMask;
        getMBTOffsetAndMask( directoryTrack, &wordOffset, &bitMask );
        pMasterBitTable[wordOffset].logicalOr( bitMask );
        ++directoryTrack;
        --tracks;
    }

    //  Finally write NOCKSM at the end, just for posterity's sake (we never do checksums).
    miscStringToWord36Fieldata( "NOCKSM", &pMasterBitTable[mbtWords - 1] );
}


//  internalPrepBuildSector0()
//
//  subfunction for internalPrep()
//  Broken out here to help avoid huge-function-syndrome
inline void
MFDManager::internalPrepBuildSector0
(
    Word36* const       pSector0,
    const SECTOR_ID     firstDirectoryTrackAddress,
    const TRACK_COUNT   directoryTracks,
    const WORD_COUNT    hmbtLength,
    const WORD_COUNT    smbtLength
)
{
    if ( directoryTracks >= 9 )
        buildSector0ForLargeDisks( pSector0, firstDirectoryTrackAddress, directoryTracks );
    else
        buildSector0ForSmallDisks( pSector0, firstDirectoryTrackAddress, hmbtLength, smbtLength );
}


//  internalPrepBuildSector1()
//
//  subfunction for internalPrep()
//  Broken out here to help avoid huge-function-syndrome
void
MFDManager::internalPrepBuildSector1
(
    Word36* const       pSector1,
    const SECTOR_ID     HMBTdeviceSectorAddress,
    const SECTOR_ID     SMBTdeviceSectorAddress,
    const TRACK_COUNT   maxAvailableTracks,         //  not counting initial reserve of directory tracks
    const std::string&  packName,
    const WORD_COUNT    mbtLengthInWords,
    const SECTOR_COUNT  dasOffset
)
{
    pSector1[0].setW( HMBTdeviceSectorAddress );
    pSector1[1].setW( SMBTdeviceSectorAddress );
    pSector1[2].setW( maxAvailableTracks );
    pSector1[3].setW( maxAvailableTracks );
    miscStringToWord36Fieldata( packName, &pSector1[4] );
    pSector1[5].setH1( 0400000 );   //  freshly-prepped fixed
    pSector1[5].setH2( static_cast<UINT32>(mbtLengthInWords) );
    pSector1[6].setT3( 016 );       //  prep status not alt-trk, written-by-exec, alt-trk-not-avail
    pSector1[010].setS3( 1 );       //  version 1
    pSector1[020].setH2( static_cast<UINT32>(dasOffset) );
}



//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------
//  constructors, destructors
//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------

MFDManager::MFDManager
(
    Exec* const         pExec
)
:ExecManager( pExec ),
m_pConsoleManager( dynamic_cast<ConsoleManager*>( pExec->getManager( Exec::MID_CONSOLE_MANAGER ) ) ),
m_pDeviceManager( dynamic_cast<DeviceManager*>( pExec->getManager( Exec::MID_DEVICE_MANAGER ) ) )
{
    m_LookupTableSize = 0;
}



//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------
//  public methods
//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------

//  allocateFileTracks()
//
//  Allocates space to an assigned file.  Space is allocated as contiguously as possible.
//  It is up to the caller to observe granularity constraints.
//
//  Parameters:
//      pActivity:          pointer to controlling Activity
//      pDiskItem:          pointer to DiskFacilityItem from which we get a pointer to a FAT
//      fileTrackId:        file-relative first track id for allocation
//      trackCount:         number of tracks for allocation
MFDManager::Result
MFDManager::allocateFileTracks
(
    Activity* const             pActivity,
    DiskFacilityItem* const     pDiskItem,
    const TRACK_ID              fileTrackId,
    const TRACK_COUNT           trackCount
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::allocateFileTracks()"
            << " RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " DSADDR=0" << std::oct << pDiskItem->getMainItem0Addr()
            << " FILE=" << pDiskItem->getQualifier() << "*" << pDiskItem->getFileName()
            << "(" << pDiskItem->getAbsoluteFileCycle() << ")"
            << " TrkId=0" << std::oct << fileTrackId
            << " TrkCount=0" << std::oct << trackCount;
        SystemLog::write( strm.str() );
    }

    Result result;

    //  Handle the trivial case...
    if ( trackCount == 0 )
        return result;

    lock();

    //  Obtain map of current allocations for the file so that we don't try to allocate space
    //  which is already allocated.  The map will be limited to the first {trackCount} tracks.
    FileAllocationTable* pFAT = pDiskItem->getFileAllocationTable();
    FileAllocationTable::FAENTRIES faEntries;
    pFAT->getFileAllocationEntries( fileTrackId, trackCount, &faEntries );

    //  Determine preferred LDATIndex.
    //  If we're cataloged, use the LDATINDEX of main item sector 0.
    //  Otherwise, use the LDATINDEX of an already-allocated track, if there is one.
    //  Failing that, just choose one at random.
    LDATINDEX preferredLDATIndex = 0;
    if ( !pDiskItem->getTemporaryFileFlag() )
        preferredLDATIndex = pDiskItem->getMainItem0Addr() >> 18;
    else if ( !pFAT->isEmpty() )
        preferredLDATIndex = pFAT->getFirstAllocation().m_LDATIndex;
    else
        preferredLDATIndex = chooseFixedLDATIndex();

    //  Build list of acceptable packs, with the preferred pack in front
    PACKINFOLIST packList;
    PackInfo* pPreferred = 0;
    for ( CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        PackInfo* pPackInfo = itpi->second;
        if ( pPackInfo->m_InFixedPool )
        {
            const DeviceManager::DeviceEntry* pEntry = m_pDeviceManager->getDeviceEntry( pPackInfo->m_DeviceId );
            if ( !pEntry )
            {
                unlock();
                std::stringstream strm;
                strm << "MFDManager::allocateFileTracks() Cannot get DeviceEntry for deviceId " << pPackInfo->m_DeviceId
                    << " for pack " << pPackInfo->m_PackName;
                SystemLog::write( strm.str() );
                result.m_Status = MFDST_INTERNAL_ERROR;
                m_pExec->stopExec( Exec::SC_DIRECTORY_ERROR );
                return result;
            }
        }

        //  Randomize the order, leaving the preferred back until later
        if ( pPackInfo->m_LDATIndex == preferredLDATIndex )
            pPreferred = pPackInfo;
        else if ( rand() > (RAND_MAX >> 1) )
            packList.push_front( pPackInfo );
        else
            packList.push_back( pPackInfo );
    }

    if ( pPreferred )
        packList.push_front( pPreferred );

    //  If nothing available, bail out
    if ( packList.empty() )
    {
        unlock();
        SystemLog::write( "MFDManager::allocateFileTracks() Out of space" );
        result.m_Status = MFDST_OUT_OF_SPACE;
        return result;
    }

    //  Iterate over the subset allocation entries, taking action for the unallocated ones.
    //  We need to do multiple allocations in the case where we cannot get contiguous entries.
    for ( FileAllocationTable::CITFAENTRIES itfae = faEntries.begin(); itfae != faEntries.end(); ++itfae )
    {
        if ( itfae->second.m_LDATIndex == 0 )
        {
            TRACK_ID workingFileTrackId = itfae->first;
            TRACK_COUNT workingTrackCount = itfae->second.m_TrackCount;
            while ( workingTrackCount > 0 )
            {
                LDATINDEX allocatedLDATIndex = 0;
                TRACK_ID deviceTrackId = 0;
                TRACK_COUNT tracksAllocated = 0;

                result = allocateFixedTracks( pActivity,
                                              packList,
                                              workingTrackCount,
                                              pDiskItem->getTemporaryFileFlag(),
                                              &allocatedLDATIndex,
                                              &deviceTrackId,
                                              &tracksAllocated );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                {
                    unlock();
                    stopExecOnResultStatus( result, false );
                    return result;
                }

                pFAT->allocated( workingFileTrackId, tracksAllocated, allocatedLDATIndex, deviceTrackId );
                workingFileTrackId += tracksAllocated;
                workingTrackCount -= tracksAllocated;
            }
        }
    }

    //  Get the new value for highest granule assigned.  We do expect the caller to behave according
    //  to granularity, so in theory, all the numbers should be on granule boundaries.  But just in case...
    bool updateMainItemSector0 = false;
    TRACK_COUNT newHighestTrackAssigned = pFAT->getHighestTrackAssigned();
    COUNT tracksPerGranule = pDiskItem->getGranularity() == MSGRAN_POSITION ? 64 : 1;
    UINT64 newHighestGranuleAssigned = newHighestTrackAssigned / tracksPerGranule;
    if ( newHighestTrackAssigned % tracksPerGranule )
        ++newHighestGranuleAssigned;
    if ( newHighestGranuleAssigned > pDiskItem->getHighestGranuleAssigned() )
    {
        pDiskItem->setHighestGranuleAssigned( static_cast<UINT32>( newHighestGranuleAssigned ) );
        updateMainItemSector0 = true;
    }

    //  If this is a cataloged file, synchronize the DAD tables in the FAT, then write them to disk.
    //  Also, if necessary, update main item sector 0.
    if ( !pDiskItem->getTemporaryFileFlag() )
    {
        pDiskItem->getFileAllocationTable()->synchronizeDADTables();
        result = writeDADUpdates( pActivity, pDiskItem->getFileAllocationTable() );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            unlock();
            stopExecOnResultStatus( result, false );
            return result;
        }

        if ( updateMainItemSector0 )
        {
            result = updateGranulesInfo( pActivity,
                                         pDiskItem->getMainItem0Addr(),
                                         pDiskItem->getInitialGranules(),
                                         pDiskItem->getMaximumGranules(),
                                         pDiskItem->getHighestGranuleAssigned(),
                                         pDiskItem->getHighestTrackWritten() );
            if ( result.m_Status != MFDST_SUCCESSFUL )
            {
                unlock();
                stopExecOnResultStatus( result, false );
                return result;
            }
        }
    }

    //TODO:REM needs attention here for updating removable MFD, if appropriate

    result = commitMFDUpdates( pActivity );
    unlock();

    stopExecOnResultStatus( result, false );
    return result;
}


//  assignFileCycle()
//
//  Updates the main item for the file cycle to indicate that it is assigned (also updates cumulative assign count).
//  For disk files, the provided FileAllocationTable is loaded based on the file's DAD entries.
//  Note: does not reload file if it is rolled out - this must be done by some higher-level entity.
//  Note: does not do anything with initial reserve - must be done by higher-level entity.
//
//  We always update the file cycle entry for newInitialReserve and newMaxGranules, so the caller should
//  grab these values via getFileCyceInfo() and pass them back to us, if no update is actually desired.
//
//  Parameters:
//      pActivity:              pointer to controlling Activity
//      mainItem0Addr:          main item sector 0 DSADDR for the file cycle to be assigned
//      exclusiveFlag:          true if file is to be made exclusive
//      newInitialReserve:      new value to be used for initial reserve
//      newMaxGranules:         new value to be used for max granules
//      ppFileAllocationTable:  where we store a pointer to the FAT for this file cycle
MFDManager::Result
MFDManager::assignFileCycle
(
    Activity* const             pActivity,
    const DSADDR                mainItem0Addr,
    const bool                  exclusiveFlag,
    const COUNT32               newInitialReserve,
    const COUNT32               newMaxGranules,
    FileAllocationTable** const ppFileAllocationTable
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::assignFileCycle() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " mainItem0Addr=0" << std::oct << mainItem0Addr
            << " EXCL=" << (exclusiveFlag ? "YES" : "NO")
            << " INIT=" << newInitialReserve
            << " MAX=" << newMaxGranules;
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    //  Read and update main item sector 0
    //  Main item 0[017].H2 is cumulative assign count
    //  Main item 0[021].T2 is current assign count
    //  Main item 0[021].bit9 is exclusive use flag
    //  Main item 0[022].W is TDATE$ current assignment started
    //  Main item 0[024].H1 is initial reserve
    //  Main item 0[025].H1 is max granules
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  Reject if file is on print queue
    if ( pMainItem0[017].getH1() != 0 )
    {
        unlock();
        result.m_Status = MFDST_FILE_ON_PRINT_QUEUE;
        return result;
    }

    pMainItem0[017].setH2( pMainItem0[017].getH2() + 1 );
    if ( exclusiveFlag )
        pMainItem0[021].setS2( pMainItem0[021].getS2() | 04 );
    COUNT newAsgCount = pMainItem0[021].getT2() + 1;
    pMainItem0[021].setT2( newAsgCount );

    TDate* pTDate = reinterpret_cast<TDate*>( &pMainItem0[022] );
    m_pExec->getExecTimeTDate( pTDate );

    pMainItem0[024].setH1( newInitialReserve );
    pMainItem0[025].setH1( newMaxGranules );

    //  Go get the lead item so we know if this is a disk file...
    DSADDR leadItem0Addr = getLinkAddress( pMainItem0[013] );
    Word36* pLeadItem0 = 0;
    if ( !stageDirectorySector( leadItem0Addr, false, &pLeadItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  Are we mass storage?  TODO:REM need to do something for removable
    if ( getFileType( pLeadItem0[011].getS1() ) == FILETYPE_MASS_STORAGE )
    {
        //  If there were no previous assignments, establish a FileAllocationTable and load it.
        if ( newAsgCount == 1 )
        {
            FileAllocationTable* pFAT = new FileAllocationTable( mainItem0Addr, false );
            m_FileAllocationDictionary[mainItem0Addr] = pFAT;
            result = loadFileAllocations( pActivity, mainItem0Addr, pFAT );
            if ( result.m_Status != MFDST_SUCCESSFUL )
            {
                unlock();
                delete pFAT;
                stopExecOnResultStatus( result, false );
                return result;
            }

            *ppFileAllocationTable = pFAT;
        }

        //  Otherwise, provide the address of the existing FileAllocationTable
        else
        {
            ITFILEALLOCATIONDICTIONARY itFad = m_FileAllocationDictionary.find( mainItem0Addr );
            if ( itFad == m_FileAllocationDictionary.end() )
            {
                SystemLog::write( "MFDManager::assignFileCycle() assign count is > 1, but no FAD in dictionary" );
                result.m_Status = MFDST_INTERNAL_ERROR;
                stopExecOnResultStatus( result, false );
                return result;
            }

            *ppFileAllocationTable = itFad->second;
        }
    }

    //  Commit the updates and we're done.
    result = commitMFDUpdates( pActivity );
    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  bringPackOnline()
//
//  Brings a pack online.  If FIXED, initialize it and add it to the fixed pool.
//  If REMOVABLE... do REM stuff.  TODO:REMOV
MFDManager::Result
    MFDManager::bringPackOnline
    (
    Activity* const                 pActivity,
    const DeviceManager::DEVICE_ID  deviceId
    )
{
    Result result;
    lock();

    //  Go read the disk label, if we can.
    PackInfo* pPackInfo = 0;
    result = readDiskLabel( pActivity, deviceId, &pPackInfo );
    unlock();

    if ( result.m_Status != MFDST_SUCCESSFUL )
        return result;

    const DeviceManager::NodeEntry* pNodeEntry = m_pDeviceManager->getNodeEntry( deviceId );
    if ( pNodeEntry == 0 )
    {
        delete pPackInfo;
        pPackInfo = 0;

        std::stringstream strm;
        strm << "MFDManager::bringPackOnline() cannot resolve deviceId " << deviceId;
        SystemLog::write( strm.str() );
        m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
        result.m_Status = MFDST_INTERNAL_ERROR;
        return result;
    }

    //  Is it fixed or removable?
    if ( pPackInfo->m_IsFixed )
    {
        //  Ask the operator if he really wants to bring the fixed pack online
        VSTRING ynResp;
        ynResp.push_back( "Y" );    //  respIndex == 0
        ynResp.push_back( "N" );    //  respIndex == 1
        std::string query = pNodeEntry->m_pNode->getName() + " TO BECOME FIXED YN?";
        INDEX respIndex;
        if ( !m_pConsoleManager->postReadReplyMessage( query, ynResp, &respIndex, pActivity->getRunInfo() ) )
        {
            delete pPackInfo;
            pPackInfo = 0;

            result.m_Status = MFDST_TERMINATING;
            return result;
        }

        if ( respIndex == 1 )
        {
            delete pPackInfo;
            pPackInfo = 0;

            std::string msg = "FIXED PACK MOUNTED ON " + pNodeEntry->m_pNode->getName() + " IGNORED";
            m_pConsoleManager->postReadOnlyMessage( msg, 0 );
            result.m_Status = MFDST_OPERATOR_ABORTED_ACTION;
            return result;
        }

        lock();
        result = bringFixedPackOnline( pActivity, pNodeEntry, pPackInfo );
        unlock();
    }
    else
    {
        //TODO:REMOV
        delete pPackInfo;   //  temporary code, which we should never reach.
    }

    return result;
}


//  cleanup()
//
//  ExecManager interface
void
    MFDManager::cleanup()
{
    //  Get rid of PackInfo stuff
    while ( m_PackInfo.size() > 0 )
    {
        delete m_PackInfo.begin()->second;
        m_PackInfo.erase( m_PackInfo.begin() );
    }

    //  Clear FileAllocationDictionary
    while ( m_FileAllocationDictionary.size() > 0 )
    {
        ITFILEALLOCATIONDICTIONARY itfad = m_FileAllocationDictionary.begin();
        delete itfad->second;
        m_FileAllocationDictionary.erase( itfad );
    }

    //  Clean cache and such
    while ( !m_DirectoryCache.empty() )
    {
        delete m_DirectoryCache.begin()->second;
        m_DirectoryCache.erase( m_DirectoryCache.begin() );
    }

    m_DirectoryTrackIdMap.clear();
    m_UpdatedSectors.clear();
}


//  createFileCycle()
//
//  Creates a file cycle in an existing fileset, for a mass-storage file.
//
//  The caller (Facilities) must do all the work ahead of time, to ensure that the requested
//  file cycle is within the allowed range of file cycles, and that a file cycle at that
//  position does not already exist.
MFDManager::Result
MFDManager::createFileCycle
(
    Activity* const         pActivity,
    const DSADDR            leadItem0Addr,
    const std::string&      accountId,
    const bool              saveOnCheckpoint,
    const bool              storeThrough,
    const bool              positionGranularity,
    const bool              wordAddressable,
    const std::string&      assignMnemonic,
    const bool              unloadInhibited,
    const bool              privateFile,
    const bool              readInhibited,
    const bool              writeInhibited,
    const UINT16            absoluteCycle,
    const COUNT32           initialReserve,
    const COUNT32           maxGranules,
    const bool              toBeCataloged
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::createFileCycle() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " leadItem0Addr=0" << std::oct << leadItem0Addr
            << " ABSCYC=" << absoluteCycle
            << " ACCT=" << accountId
            << " MNEM=" << assignMnemonic
            << " UNLINH=" << (unloadInhibited ? "YES" : "NO")
            << " PRIV=" << (privateFile ? "YES" : "NO")
            << " RINH=" << (readInhibited ? "YES" : "NO")
            << " WINH=" << (writeInhibited ? "YES" : "NO")
            << " INIT=" << initialReserve
            << " MAX=" << maxGranules
            << " TOBOCAT=" << (toBeCataloged ? "YES" : "NO");
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    //  Stage lead item to pick up some useful information
    Word36* pLeadItem0 = 0;
    if ( !stageDirectorySector( leadItem0Addr, false, &pLeadItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    bool guardedFile = (pLeadItem0[012].getS1() & 040) == 040 ? true : false;

    {//TODO:DEBUG
        std::string qualifier = miscWord36FieldataToString( &pLeadItem0[1], 2 );
        std::string filename = miscWord36FieldataToString( &pLeadItem0[3], 2 );
        std::string projectId = miscWord36FieldataToString( &pLeadItem0[5], 2 );
        std::stringstream strm;
        strm << "     FILE=" << qualifier << "*" << filename
            << " PROJ=" << projectId
            << " GUARD=" << (guardedFile ? "YES" : "NO");
        SystemLog::write( strm.str() );
    }

    //  Allocate DSADDR's for main item sectors 0 and 1.
    DSADDR mainItem0Addr = 0;
    LDATINDEX preferredLDATIndex = (leadItem0Addr >> 18) & 07777;
    result = allocateDirectorySector( pActivity, preferredLDATIndex, &mainItem0Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    DSADDR mainItem1Addr = 0;
    result = allocateDirectorySector( pActivity, preferredLDATIndex, &mainItem1Addr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  stage the newly-allocated main items - no good reason to test existence; we've just now allocated them.
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
        return result;

    Word36* pMainItem1 = 0;
    if ( !stageDirectorySector( mainItem1Addr, true, &pMainItem1, &result ) )
        return result;

    //  Create main item sector 0
    pMainItem0[0].setW( 0600000000000 );
    pMainItem0[1] = pLeadItem0[1];
    pMainItem0[2] = pLeadItem0[2];
    pMainItem0[3] = pLeadItem0[3];
    pMainItem0[4] = pLeadItem0[4];
    pMainItem0[5] = pLeadItem0[5];
    pMainItem0[6] = pLeadItem0[6];
    miscStringToWord36Fieldata( accountId, &pMainItem0[7], 2 );

    pMainItem0[013].setW( leadItem0Addr );

    if ( toBeCataloged )
        pMainItem0[014].setT1( 010 );
    if ( storeThrough )
        pMainItem0[014].setS3( 01 );

    pMainItem0[015].setW( mainItem1Addr );
    UINT32 flags = 0;
    if ( positionGranularity )
        flags |= 040;
    if ( wordAddressable )
        flags |= 010;
    pMainItem0[015].setS1( flags );

    miscStringToWord36Fieldata( assignMnemonic, &pMainItem0[016], 1 );

    UINT32 inhibit = 0;
    if ( guardedFile )
        inhibit |= 040;
    if ( unloadInhibited )
        inhibit |= 020;
    if ( privateFile )
        inhibit |= 010;
    if ( readInhibited )
        inhibit |= 002;
    if ( writeInhibited )
        inhibit |= 001;
    pMainItem0[021].setS2( inhibit );
    pMainItem0[021].setT3( absoluteCycle );

    TDate* pTDate = reinterpret_cast<TDate*>( &pMainItem0[023] );
    m_pExec->getExecTimeTDate( pTDate );

    pMainItem0[024].setH1( initialReserve );
    pMainItem0[025].setH1( maxGranules );

    //  Create main item sector 1
    pMainItem1[0].setW( 0400000000000 );
    for ( INDEX wx = 1; wx < 5; ++wx )
        pMainItem1[wx] = pMainItem0[wx];
    miscStringToWord36Fieldata( "*No.1*", &pMainItem1[05], 1 );
    pMainItem1[06].setW( mainItem0Addr );
    pMainItem1[07].setT3( absoluteCycle );
    pMainItem1[011].setS1( 1 );

    result = insertMainItem( pActivity, leadItem0Addr, mainItem0Addr, absoluteCycle, guardedFile );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    result = commitMFDUpdates( pActivity );
    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  createFileSet()
//
//  Creates an empty file set (has no file cycles).
//  Should only be used in conjunction with a subsequent call to create a file cycle.
MFDManager::Result
    MFDManager::createFileSet
    (
    Activity* const             pActivity,
    const std::string&          qualifier,
    const std::string&          filename,
    const std::string&          projectId,
    const std::string&          readKey,
    const std::string&          writeKey,
    const FileType              fileType,
    const bool                  guardedFile,
    DSADDR* const               pDSAddr
    )
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::createFileSet() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " FILE=" << qualifier << "*" << filename << "/" << readKey << "/" << writeKey
            << " PROJ=" << projectId
            << " TYPE=" << MFDManager::getFileTypeString( fileType )
            << " GOPT=" << (guardedFile ? "YES" : "NO");
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    //  Get a new directory sector
    LDATINDEX preferredLDATIndex = chooseFixedLDATIndex();
    DSADDR leadItemAddr = 0;
    result = allocateDirectorySector( pActivity, preferredLDATIndex, &leadItemAddr );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  Create Lead item sector 0 (there will be no sector 1 at this point)
    Word36* pLeadItem = 0;
    if ( stageDirectorySector( leadItemAddr, true, &pLeadItem, &result ) )
    {
        pLeadItem[0].setW( 0500000000000ll );
        miscStringToWord36Fieldata( qualifier, &pLeadItem[1], 2 );
        miscStringToWord36Fieldata( filename, &pLeadItem[3], 2 );
        miscStringToWord36Fieldata( projectId, &pLeadItem[5], 2 );
        if ( readKey.size() > 0 )
            miscStringToWord36Fieldata( readKey, &pLeadItem[7], 1 );
        if ( writeKey.size() > 0 )
            miscStringToWord36Fieldata( writeKey, &pLeadItem[010], 1 );
        if ( fileType == FILETYPE_TAPE )
            pLeadItem[011].setS1( 01 );
        pLeadItem[011].setS3( 32 );             // max file cycles allowed
        if ( guardedFile )
            pLeadItem[012].setT1( 04000 );

        //  Link the new leaditem to the search table.
        result = establishLookupEntry( pActivity, &pLeadItem[1], &pLeadItem[3], leadItemAddr );
        if ( result.m_Status == MFDST_SUCCESSFUL )
        {
            *pDSAddr = leadItemAddr;
            result = commitMFDUpdates( pActivity );
        }
    }

    unlock();
    stopExecOnResultStatus( result, false );
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::createFileSet() returning ";
        if ( result.m_Status == MFDST_SUCCESSFUL )
            strm << "DSADDR=0" << std::oct << (*pDSAddr);
        else
            strm << "MFDResult:" << MFDManager::getResultString( result );
        SystemLog::write( strm.str() );
    }

    return result;
}


//  dropFileCycle()
//
//  Drops a file cycle.
//  For mass storage files, we drop all the related DAD tables and deallocate the associated tracks.
//  For tape files, we drop all the related reel tables.
//  Drops all other related sectors as well.
//
//  Observes file assignment counter and SMOQUE flag, so that we do not yank a file cycle out from
//  under another run, or the output queue manager.
MFDManager::Result
MFDManager::dropFileCycle
(
    Activity* const         pActivity,
    const DSADDR            mainItem0Addr,
    const bool              commitFlag
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::dropFileCycle() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " mainItem0Addr=0" << std::oct << mainItem0Addr
            << " commit=" << (commitFlag ? "TRUE" : "FALSE");
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    //  Read main item 0
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, false, &pMainItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  Is the file on SMOQUE?  Check word 15,h1 - zero means not on queue
    if ( pMainItem0[017].getH1() != 0 )
    {
        unlock();
        result.m_Status = MFDST_FILE_ON_PRINT_QUEUE;
        return result;
    }

    //  Check file assign count.  If it is assigned set it to-be-deleted.
    //  This means that FacilityManager must release a file from the run requesting deletion,
    //  before invoking this routine, or else, we'd set to-be-deleted when we should just delete.
    //  File assign count is in word 17,T2, to-be-deleted is Word 12 bit 11.
    if ( pMainItem0[021].getT2() != 0 )
    {
        //  Do this only if it's not already to-be-deleted.
        if ( (pMainItem0[014].getS2() & 01) == 0 )
        {
            stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result );
            pMainItem0[014].logicalOr( 0010000000000ll );
        }

        return result;
    }

    //  Read lead items 0 and possibly 1 (we'll need this for file type, and also to update later)
    DSADDR leadItem0Addr = getLinkAddress( pMainItem0[013] );
    DSADDR leadItem1Addr = 0;
    Word36* pLeadItem0 = 0;
    Word36* pLeadItem1 = 0;
    if ( !stageLeadItems( leadItem0Addr, true, &leadItem1Addr, &pLeadItem0, &pLeadItem1, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    UINT8 fileType = pLeadItem0[011].getS1();
    result = dropFileCycleInternal( pActivity, mainItem0Addr, fileType );
    if ( result.m_Status != MFDST_SUCCESSFUL )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  Find main item link.  We need the entry index as well as the pointer.
    COUNT currentRange = pLeadItem0[011].getQ3();
    Word36* pLink = 0;
    INDEX ex = 0;
    for ( ; ex < currentRange; ++ex )
    {
        pLink = getMainItemLinkPointer( ex, pLeadItem0, pLeadItem1 );
        if ( getLinkAddress( *pLink ) )
            break;
    }

    //  Clear the link
    pLink->setW( 0 );

    //  If this was the highest absolute cycle, we need to move some cycles upward
    //  and update the highest absolute fcycle.  Also, if +1 exists is set, clear it.
    //  (highest abs cycle is word 011,Q4 - +1 exists is 012,T1,Bit1)
    if ( ex == 0 )
    {
        //  If current range is 1, we're dropping the only existing entry, so no need to shift...
        if ( currentRange == 1 )
        {
            //  Just clear the current range value.
            pLeadItem0[011].setW( 0 );
        }
        else
        {
            //  Find first non-empty link...
            INDEX sx = ex + 1;
            while ( sx < currentRange )
            {
                Word36* pCheck = getMainItemLinkPointer( sx, pLeadItem0, pLeadItem1 );
                if ( !pCheck->isZero() )
                    break;
            }

            //  Do the shift
            INDEX dx = 0;
            while ( sx < currentRange )
            {
                Word36* pSource = getMainItemLinkPointer( sx, pLeadItem0, pLeadItem1 );
                Word36* pDest = getMainItemLinkPointer( dx, pLeadItem0, pLeadItem1 );
                *pDest = *pSource;
                ++sx;
                ++dx;
            }

            while ( dx < currentRange )
            {
                Word36* pDest = getMainItemLinkPointer( dx, pLeadItem0, pLeadItem1 );
                pDest->setW( 0 );
                ++dx;
            }
        }

        //  The dropped cycle may or may not be +1, but either way, clear it.
        pLeadItem0[012].logicalAnd( 0500000000000ll );
    }

    //  Update lead item 0's current range and cycle count
    --currentRange;
    if ( currentRange > 0 )
        calculateCycleInfo( pLeadItem0, pLeadItem1 );

    //  If this set is guarded, check all the still-existing cycles to see if we still need to be guarded.
    if ( (currentRange > 0) && (pLeadItem0[012].getT1() & 04000) )
    {
        result = updateGuardedBit( pActivity, leadItem0Addr, pLeadItem0, pLeadItem1 );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            unlock();
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    //  If there are no more cycles, drop the set.
    if ( currentRange == 0 )
        result = dropFileSetInternal( pActivity, leadItem0Addr );

    if ( commitFlag )
        commitMFDUpdates( pActivity );

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  dropFileSet()
//
//  Drops an entire file set.
//  Up to caller (Facilities) to ensure none of the existing cycles are currently assigned, queued, etc.
//  In practice, this is only called in order to back out an operation involving creating a file set,
//  which encountered an error (during @CAT), so there's not likely to be an issue in this regard.
MFDManager::Result
MFDManager::dropFileSet
(
    Activity* const         pActivity,
    const DSADDR            leadItem0Addr
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::dropFileSet() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " leadItem0Addr=0" << std::oct << leadItem0Addr;
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    DSADDR leadItem1Addr = 0;
    Word36* pLeadItem0 = 0;
    Word36* pLeadItem1 = 0;
    if ( !stageLeadItems( leadItem0Addr, false, &leadItem1Addr, &pLeadItem0, &pLeadItem1, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    //  If fileset is not empty, drop the individual file cycles first
    COUNT currentRange = pLeadItem0[011].getQ3();
    if ( currentRange > 0 )
    {
        for ( INDEX ex = 0; ex < currentRange; ++ex )
        {
            Word36* pLink = getMainItemLinkPointer( ex, pLeadItem0, pLeadItem1 );
            DSADDR mainItem0Addr = getLinkAddress( *pLink );
            if ( mainItem0Addr != 0 )
            {
                result = dropFileCycle( pActivity, mainItem0Addr, false );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                {
                    unlock();
                    stopExecOnResultStatus( result, false );
                    return result;
                }
            }
        }
    }

    //  Now drop the set
    result = dropFileSetInternal( pActivity, leadItem0Addr );

    //  Commit the datastore transaction
    result = commitMFDUpdates( pActivity );
    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  dump()
//
//  For debugging
void
MFDManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBitMask
)
{
    lock();

    stream << "MFDManager ----------" << std::endl;
    stream << "  Lookup Table Size:     " << std::dec << m_LookupTableSize << std::endl;
    stream << "  Overhead Account Id:   " << m_OverheadAccountId << std::endl;
    stream << "  Overhead User Id:      " << m_OverheadUserId << std::endl;

    if ( dumpBitMask & DUMP_TRACK_ID_MAP )
    {
        stream << "  MFD-relative Track ID to Block ID map" << std::endl;
        for ( CITDIRECTORYTRACKIDMAP it = m_DirectoryTrackIdMap.begin(); it != m_DirectoryTrackIdMap.end(); ++it )
        {
            stream << "    " << std::setw( 10 ) << std::oct << std::setfill( '0' ) << it->first
                << " -> " << std::setw( 10 ) << std::oct << std::setfill( '0' ) << it->second << std::endl;
        }
    }

    if ( dumpBitMask & DUMP_CACHED_DIRECTORY_SECTOR_DATA )
    {
        stream << "  Cached Directory Sectors:" << std::endl;
        for ( CITDIRECTORYCACHE itdc = m_DirectoryCache.begin(); itdc != m_DirectoryCache.end(); ++itdc )
        {
            std::stringstream leadStrm;
            leadStrm << "    " << std::oct << std::setw(10) << std::setfill('0') << itdc->first << ":";
            COUNT leadSize = leadStrm.str().size();
            std::string leadStr = leadStrm.str();
            for ( INDEX wx = 0; wx < 28; wx += 4 )
            {
                stream << leadStr;
                if ( wx == 0 )
                {
                    leadStr.clear();
                    leadStr.resize( leadSize );
                }

                for ( INDEX wy = 0; wy < 4; ++wy )
                    stream << " " << itdc->second[wx + wy].toOctal();
                stream << " ";
                for ( INDEX wy = 0; wy < 4; ++wy )
                    stream << " " << itdc->second[wx + wy].toFieldata();
                stream << " ";
                for ( INDEX wy = 0; wy < 4; ++wy )
                    stream << " " << itdc->second[wx + wy].toAscii();
                stream << std::endl;
            }
        }

        stream << "  Updated Directory Sectors:" << std::endl;
        for ( CITDSADDRSET itus = m_UpdatedSectors.begin(); itus != m_UpdatedSectors.end(); ++itus )
            stream << "    0" << std::oct << *itus << std::endl;
    }

    if ( dumpBitMask & DUMP_FILE_ALLOCATION_TABLE )
    {
        stream << "  File Allocation Directory:" << std::endl;
        for ( CITFILEALLOCATIONDICTIONARY itfad = m_FileAllocationDictionary.begin(); itfad != m_FileAllocationDictionary.end(); ++itfad )
        {
            std::stringstream fileIdStrm;
            fileIdStrm << "DSADDR=" << std::oct << std::setw(10) << std::setfill('0') << itfad->first;
            itfad->second->dump( stream, "      ", fileIdStrm.str() );
        }
    }

    stream << "  Pack Info:" << std::endl;
    stream << "    LDAT  PackId  Prep      DirTrkSector  HMBTWds  SMBTWds  Tracks        Mntd  DvId  FxPl  Asg#" << std::endl;
    for ( CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        std::string padPackId = itpi->second->m_PackName;
        padPackId.resize( 6, ' ' );

        stream << "    " << std::oct << std::setw(4) << std::setfill('0') << itpi->second->m_LDATIndex
                << "  " << padPackId
                << "  " << std::dec << std::setw(4) << itpi->second->m_PrepFactor
                << "  " << ( itpi->second->m_IsFixed ? " F" : " R" )
                << "  " << std::oct << std::setw(12) << std::setfill('0') << itpi->second->m_DirectoryTrackAddress
                << "  " << std::oct << std::setw(7) << std::setfill('0') << itpi->second->m_S0S1HMBTPadWords
                << "  " << std::oct << std::setw(7) << std::setfill('0') << itpi->second->m_SMBTWords
                << "  " << std::oct << std::setw(12) << std::setfill('0') << itpi->second->m_TotalTracks
                << "  " << (itpi->second->m_IsMounted ? " Y  " : " N  ")
                << "  " << std::dec << std::setw(4) << itpi->second->m_DeviceId
                << "  " << (itpi->second->m_InFixedPool ? " Y  " : " N  ")
                << "  " << std::dec << std::setw(4) << itpi->second->m_AssignCount
                << std::endl;

        if ( dumpBitMask & DUMP_DISK_ALLOCATIONS )
            itpi->second->m_DiskAllocationTable.dump( stream, "      ", itpi->second->m_PackName );
    }

    if ( dumpBitMask & DUMP_SEARCH_ITEM_LOOKUP_TABLE )
    {
        stream << "  Search Item Lookup Table:  HASH          DSADDR" << std::endl;
        for ( INDEX sx = 0; sx < m_SearchItemLookupTable.size(); ++sx )
        {
            if ( m_SearchItemLookupTable[sx] != 0 )
            {
                stream << "                             "
                        << std::oct << std::setw(12) << std::setfill('0') << sx
                        << "  " << std::oct << std::setw(10) << std::setfill('0') << m_SearchItemLookupTable[sx]
                        << std::endl;
            }
        }
    }

    if ( dumpBitMask & DUMP_MFD_DIRECTORY )
        dumpMFDDirectory( stream );

    unlock();
}


//  getAssignedNodeIds()
//
//  Populate the given container with the NODE_ID values of the devices which host a pack which is assigned.
void
MFDManager::getAssignedNodeIds
(
    DeviceManager::NODE_IDS* const  pContainer
) const
{
    pContainer->clear();

    lock();
    for ( CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        if ( itpi->second->m_AssignCount > 0 )
            pContainer->push_back( itpi->second->m_DeviceId );
    }
    unlock();
}


//  getDeviceId()
//
//  Converts LDATINDEX to DeviceId for IoManager and anyone else who needs this.
//  Returns false if the ldat is not mounted, or not found.
bool
MFDManager::getDeviceId
(
    const LDATINDEX                 ldatIndex,
    DeviceManager::DEVICE_ID* const pDeviceId
) const
{
    bool result = false;
    lock();

    CITPACKINFOMAP itpi = m_PackInfo.find( ldatIndex );
    if ( (itpi != m_PackInfo.end()) && itpi->second->m_IsMounted )
    {
        *pDeviceId = itpi->second->m_DeviceId;
        result = true;
    }

    unlock();
    return result;
}


//  getDirectorySector()
//
//  Retrieves a single directory sector
MFDManager::Result
MFDManager::getDirectorySector
(
    const DSADDR        directorySectorAddress,
    Word36* const       pBuffer
) const
{
    Result result;
    lock();

    Word36* pCacheBuffer = 0;
    if ( stageDirectorySector( directorySectorAddress, &pCacheBuffer, &result ) )
    {
        for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; ++wx )
            pBuffer[wx] = pCacheBuffer[wx];
    }

    unlock();
    return result;
}


//  getFileCycleInfo()
//
//  Populates a FileCycleInfo object for the indicated main item address.
MFDManager::Result
MFDManager::getFileCycleInfo
(
    const DSADDR            mainItem0Addr,
    FileCycleInfo* const    pInfo
)
{
    Result result;
    lock();

    Word36* pMainItem0 = 0;
    Word36* pMainItem1 = 0;
    DSADDR mainItem1Addr;
    if ( stageMainItems( mainItem0Addr, false, &mainItem1Addr, &pMainItem0, &pMainItem1, &result ) )
        getCommonFileCycleInfo( pInfo, mainItem0Addr, pMainItem0, mainItem1Addr, pMainItem1 );

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  getFileSetInfo()
//
//  Populates a FileSetInfo object for the indicated qualifier/filename combination.
MFDManager::Result
MFDManager::getFileSetInfo
(
    const std::string&      qualifier,
    const std::string&      filename,
    FileSetInfo* const      pInfo
)
{
    Result result;
    lock();

    //  Find the address to lead item 0 for the qual/file combination
    Word36 qualifier36[2];
    Word36 filename36[2];
    miscStringToWord36Fieldata( qualifier, &qualifier36[0], 2 );
    miscStringToWord36Fieldata( filename, &filename36[0], 2 );
    DSADDR leadItemAddr0 = getLeadItemAddress( &qualifier36[0], &filename36[0] );
    if ( leadItemAddr0 == 0 )
    {
        unlock();
        result.m_Status = MFDST_NOT_FOUND;
        return result;
    }

    result = getFileSetInfo( leadItemAddr0, pInfo );

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  getFileSetInfoList()
//
//  Populates a FileSetInfoList object for all cataloged file sets
MFDManager::Result
MFDManager::getFileSetInfoList
(
    FileSetInfoList* const  pList
)
{
    Result result;
    lock();

    //  Iterate over the search item lookup table
    Word36* pSearchEntry = 0;
    Word36* pSearchItem = 0;
    for ( INDEX sx = 0; sx < m_SearchItemLookupTable.size(); ++sx )
    {
        //  Found an item (which might be at the head of a chain).  Iterate over the chain.
        DSADDR searchItemAddr = m_SearchItemLookupTable[sx];
        while ( searchItemAddr != 0 )
        {
            //  Stage the search item.
            if ( !stageDirectorySector( searchItemAddr, &pSearchItem, &result ) )
            {
                unlock();
                return result;
            }

            //  Iterate over the potential entries in this search item
            pSearchEntry = pSearchItem + 1;
            for ( INDEX ex = 0; ex < 5; ++ex )
            {
                DSADDR leadItemAddr0 = getLinkAddress( pSearchEntry[4] );
                if ( leadItemAddr0 != 0 )
                {
                    //  We have a lead item address - load a file set info object for it.
                    FileSetInfo* pfsInfo = new FileSetInfo();
                    result = getFileSetInfo( leadItemAddr0, pfsInfo );
                    if ( result.m_Status != MFDST_SUCCESSFUL )
                    {
                        delete pfsInfo;
                        unlock();
                        return result;
                    }

                    pList->push_back( pfsInfo );
                }

                pSearchEntry += 5;
            }

            //  Get next linked search item
            searchItemAddr = (result.m_Status == MFDST_SUCCESSFUL) ? getLinkAddress( pSearchItem[0] ) : 0;
        }
    }

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  getFixedPoolTrackCounts()
//
//  Retrieves the number of tracks accessible on all UP and SU fixed packs,
//  and the number of tracks available (unassigned) on all UP fixed packs.
//  Used by MS keyin.
MFDManager::Result
    MFDManager::getFixedPoolTrackCounts
    (
    TRACK_COUNT* const      pAccessible,
    TRACK_COUNT* const      pAvailable
    ) const
{
    Result result;
    *pAccessible = 0;
    *pAvailable = 0;

    lock();

    //  Iterate over the packs
    for ( CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        PackInfo* pPackInfo = itpi->second;
        if ( pPackInfo->m_InFixedPool )
        {
            TRACK_COUNT packAccessible = 0;
            TRACK_COUNT packAvailable = 0;

            result = getPackTrackCounts( pPackInfo, &packAccessible, &packAvailable );
            if ( result.m_Status != MFDST_SUCCESSFUL )
                break;

            (*pAccessible) += packAccessible;
            const DeviceManager::DeviceEntry* pEntry = m_pDeviceManager->getDeviceEntry( pPackInfo->m_DeviceId );
            if ( pEntry == 0 )
            {
                result.m_Status = MFDST_INTERNAL_ERROR;
                break;
            }

            if ( pEntry->m_Status == DeviceManager::NDST_UP )
                (*pAvailable) += packAvailable;
        }
    }

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  getMassStorageFileCycleInfo()
//
//  Populates a MassStorageFileCycleInfo object for the indicated main item address,
//  which is expected to reflect a mass storage cataloged file.
MFDManager::Result
MFDManager::getMassStorageFileCycleInfo
(
    const DSADDR                    mainItem0Addr,
    MassStorageFileCycleInfo* const pInfo
)
{
    Result result;
    lock();

    Word36* pMainItem0 = 0;
    Word36* pMainItem1 = 0;
    DSADDR mainItem1Addr;
    if ( stageMainItems( mainItem0Addr, false, &mainItem1Addr, &pMainItem0, &pMainItem1, &result ) )
    {
        FileCycleInfo* pCommonInfo = dynamic_cast<FileCycleInfo*>( pInfo );
        getCommonFileCycleInfo( pCommonInfo, mainItem0Addr, pMainItem0, mainItem1Addr, pMainItem1 );

        //  Mass-storage-specific
        UINT16 descriptors = pMainItem0[014].getT1();
        UINT8 inhibits = pMainItem0[021].getS2();
        UINT8 fileFlags = pMainItem0[014].getS3();
        UINT8 pchar = pMainItem0[015].getS1();

        pInfo->m_TDateFirstWriteAfterBackup = pMainItem0[012];
        pInfo->m_Unloaded = (descriptors & 04000) ? true : false;
        pInfo->m_SaveOnCheckpoint = (descriptors & 01000) ? true : false;
        pInfo->m_LargeFile = (fileFlags & 040) ? true : false;
        pInfo->m_WrittenTo = (fileFlags & 002) ? true : false;
        pInfo->m_StoreThrough = (fileFlags & 001) ? true : false;
        pInfo->m_PositionGranularity = (pchar & 040) ? true : false;
        pInfo->m_WordAddressable = (pchar & 010) ? true : false;
        pInfo->m_AssignedToCommonNameSection = (pchar & 004) ? true : false;
        pInfo->m_UnloadInhibited = (inhibits & 020) ? true : false;
        pInfo->m_InitialReserve = pMainItem0[024].getH1();
        pInfo->m_MaxGranules = pMainItem0[025].getH1();
        pInfo->m_HighestGranuleAssigned = pMainItem0[026].getH1();
        pInfo->m_HighestTrackWritten = pMainItem0[027].getH1();
    }

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  getPackInfo()
//
//  Retrieves a pointer to immutable PackInfo object given the device identifier for the
//  disk unit upon which the pack is mounted.
const MFDManager::PackInfo*
    MFDManager::getPackInfo
    (
    const DeviceManager::DEVICE_ID  deviceId
    ) const
{
    PackInfo* pPackInfo = 0;
    lock();
    for ( CITPACKINFOMAP itpi = m_PackInfo.begin(); itpi != m_PackInfo.end(); ++itpi )
    {
        if ( itpi->second->m_DeviceId == deviceId )
        {
            pPackInfo = itpi->second;
            break;
        }
    }

    unlock();
    return pPackInfo;
}


//  getPackTrackCounts()
//
//  Retrieve accessible and available track counts for the indicated pack.
//  Used by FS,AVAIL and (indirectly) by MS keyins.
MFDManager::Result
    MFDManager::getPackTrackCounts
    (
    const PackInfo* const       pPackInfo,
    TRACK_COUNT* const          pAccessible,
    TRACK_COUNT* const          pAvailable
    ) const
{
    Result result;

    lock();

    DSADDR dsAddr = (pPackInfo->m_LDATIndex << 18) | 01;
    Word36* pSector1 = 0;
    if ( stageDirectorySector( dsAddr, &pSector1, &result ) )
    {
        //TODO:REM Does this work for removable?
        *pAccessible = pSector1[02].getW();
        *pAvailable = pSector1[03].getW();
    }

    unlock();
    return result;
}


//  getPrepFactor()
//
//  Gets prepfactor of pack indicated by the given LDATINDEX - for IoManager
bool
MFDManager::getPrepFactor
(
    const LDATINDEX     ldatIndex,
    PREP_FACTOR* const  pPrepFactor
) const
{
    bool result = false;
    lock();

    CITPACKINFOMAP itpi = m_PackInfo.find( ldatIndex );
    if ( (itpi != m_PackInfo.end()) && itpi->second->m_IsMounted )
    {
        *pPrepFactor = itpi->second->m_PrepFactor;
        result = true;
    }

    unlock();
    return result;
}


//  getSearchItemAddresses()
//
//  Mainly for @DIR - retrieves a set containing all of the DSADDR's on a particular search item chain
MFDManager::Result
MFDManager::getSearchItemAddresses
(
    const SuperString&  qualifier,
    const SuperString&  filename,
    DSADDRLIST*         pAddresses
) const
{
    Result result;
    pAddresses->clear();

    lock();

    DSADDR searchItemAddr = getSearchItemAddress( qualifier, filename );
    Word36* pSearchItem = 0;
    while ( searchItemAddr != 0 )
    {
        pAddresses->push_back( searchItemAddr );
        if ( !stageDirectorySector( searchItemAddr, &pSearchItem, &result ) )
            break;
        searchItemAddr = getLinkAddress( pSearchItem[0] );
    }

    unlock();

    if ( pAddresses->empty() && (result.m_Status == MFDST_SUCCESSFUL) )
        result.m_Status = MFDST_NOT_FOUND;
    return result;
}


//  getTapeFileCycleInfo()
//
//  Populates a TapeFileCycleInfo object for the indicated main item address,
//  which is expected to reflect a mass storage cataloged file.
MFDManager::Result
MFDManager::getTapeFileCycleInfo
(
    const DSADDR                mainItem0Addr,
    TapeFileCycleInfo* const    pInfo
)
{
    Result result;
    lock();

    Word36* pMainItem0 = 0;
    Word36* pMainItem1 = 0;
    DSADDR mainItem1Addr;
    if ( stageMainItems( mainItem0Addr, false, &mainItem1Addr, &pMainItem0, &pMainItem1, &result ) )
    {
        FileCycleInfo* pCommonInfo = dynamic_cast<FileCycleInfo*>( pInfo );
        getCommonFileCycleInfo( pCommonInfo, mainItem0Addr, pMainItem0, mainItem1Addr, pMainItem1 );

        //  Tape-specific
        UINT8 density = pMainItem0[024].getS1();
        UINT8 format = pMainItem0[024].getS2();
        UINT8 features = pMainItem0[024].getS3();
        UINT8 mtapop = pMainItem0[025].getS3();

        if ( features & 040 )
            pInfo->m_BlockNumbering = TBN_ON;
        else if ( features & 010 )
            pInfo->m_BlockNumbering = TBN_OPTIONAL;
        else
            pInfo->m_BlockNumbering = TBN_OFF;

        if ( features & 004 )
            pInfo->m_DataCompression = TDC_ON;
        else if ( features & 001 )
            pInfo->m_DataCompression = TDC_OPTIONAL;
        else
            pInfo->m_DataCompression = TDC_OFF;

        switch ( density )
        {
        case 001:
            if ( mtapop & 010 ) // HIC
                pInfo->m_Density = TDENS_38000;
            else
                pInfo->m_Density = TDENS_800;
            break;
        case 002:
            pInfo->m_Density = TDENS_1600;
            break;
        case 003:
            pInfo->m_Density = TDENS_6250;
            break;
        case 004:
            pInfo->m_Density = TDENS_5090;
            break;
        case 005:
            pInfo->m_Density = TDENS_38000;
            break;
        case 006:
            pInfo->m_Density = TDENS_76000;
            break;
        case 007:
            pInfo->m_Density = TDENS_85937;
            break;
        }

        if ( format & 002 )
            pInfo->m_Format = TFMT_QUARTER_WORD;
        else if ( format & 004 )
            pInfo->m_Format = TFMT_SIX_BIT_PACKED;
        else
            pInfo->m_Format = TFMT_EIGHT_BIT_PACKED;

        if ( mtapop & 020 )
            pInfo->m_Type = TTYPE_QIC;
        else if ( mtapop & 010 )
            pInfo->m_Type = TTYPE_HIC;
        else if ( mtapop & 002 )
            pInfo->m_Type = TTYPE_DLT;
        else if ( mtapop & 001 )
            pInfo->m_Type = TTYPE_HIS;
        else
        {
            if ( format & 040 )
                pInfo->m_Type = TTYPE_NINE_TRACK;
            else
                pInfo->m_Type = TTYPE_SEVEN_TRACK;
        }

        if ( (pInfo->m_Type == TTYPE_SEVEN_TRACK) || (pInfo->m_Type == TTYPE_NINE_TRACK) )
        {
            if ( format & 020 )
                pInfo->m_Parity = TPAR_EVEN;
            else
                pInfo->m_Parity = TPAR_ODD;
        }
        else
            pInfo->m_Parity = TPAR_NONE;

        pInfo->m_NoiseConstant = pMainItem0[025].getS6();
        pInfo->m_JOption = ( mtapop & 040 ) ? true : false;

        if ( pMainItem0[032].getW() != 0 )
            pInfo->m_Reels.push_back( miscWord36FieldataToString( &pMainItem0[032], 1 ));
        if ( pMainItem0[033].getW() != 0 )
            pInfo->m_Reels.push_back( miscWord36FieldataToString( &pMainItem0[033], 1 ));
        DSADDR reelTableAddr = getLinkAddress( pMainItem0[0] );
        while ( reelTableAddr )
        {
            Word36* pReelTable = 0;
            if ( !stageDirectorySector( reelTableAddr, false, &pReelTable, &result ) )
                break;

            pInfo->m_DirectorySectorAddresses.push_back( reelTableAddr );

            for ( INDEX wx = 2; wx < 033; ++wx )
            {
                if ( pReelTable[wx].getW() != 0 )
                    pInfo->m_Reels.push_back( miscWord36FieldataToString( &pReelTable[wx], 1 ));
            }

            reelTableAddr = getLinkAddress( pReelTable[0] );
        }
    }

    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  initialize()
//
//  Initializes the MFD manager.
//  Any non-successful return from here must be preceded by stopping the EXEC
//  (except for MFDST_TERMINATING, which implies exec is stopping anyway).
MFDManager::Result
MFDManager::initialize
(
    Activity* const     pActivity
)
{
    Result result;

    //  Clear out pack info table
    while ( !m_PackInfo.empty() )
    {
        delete m_PackInfo.begin()->second;
        m_PackInfo.erase( m_PackInfo.begin()->first );
    }

    //  Iterate over the boot disk info we've collected, for all fixed packs.
    //  Assign LDAT indexes, and build m_PackInfo for all packs.
    //  DNs any packs with pack name conflicts (this means we'll still always have at least one).
    if ( !pActivity->isTerminating() )
        initializeLoadPackInfo( pActivity, m_BootPackInfo );

    //  Initialize fixed packs..
    //  This clears out all allocations for the fixed packs using direct IO.
    //  Also, it loads directory sectors into cache.
    if ( !pActivity->isTerminating() )
    {
        result = initializeFixedPacks( pActivity );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    //  Load Disk Allocation Tables for the fixed packs based on the SMBTs
    if ( !pActivity->isTerminating() )
    {
        result = loadFixedPackAllocationTables( pActivity );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    //  Manually catalog SYS$*MFDF$$ (by creating directory sectors).
    //  This function creates a search item, lead item, and main items,
    //  as well as updating the search item lookup table and DAD entries.
    DSADDR mfdMainItem0Addr;
    FileAllocationTable* pMFDFat = 0;
    if ( !pActivity->isTerminating() )
    {
        result = initializeCatalogMFD( pActivity, &mfdMainItem0Addr, &pMFDFat );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    //  Manually assign SYS$*MFDF$$ (create a FacItem and stick in in Exec's RunInfo)
    if ( !pActivity->isTerminating() )
    {
        result = initializeAssignMFD( pActivity, mfdMainItem0Addr, pMFDFat );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    //  Commit MFD changes (now that we have the MFD assigned and its DAD tables loaded)
    if ( !pActivity->isTerminating() )
    {
        result = commitMFDUpdates( pActivity );
        if ( result.m_Status != MFDST_SUCCESSFUL )
        {
            stopExecOnResultStatus( result, false );
            return result;
        }
    }

    return result;
}


//  prepDisk()
//
//  Allows external callers (i.e., PREPKeyin) to ask for a pack to be prepped.
//  Pack must be formatted, but it does not need to have a label - we'll label it.
//  Pack should be RV'd, but we do not force this here.
//  This results in the possibility of IO to a logically DN (or UP) pack - so be careful out there...
MFDManager::Result
MFDManager::prepDisk
(
    Activity* const               pActivity,
    const DeviceManager::NODE_ID  nodeId,
    const bool                    fixedFlag,
    const SuperString&            packLabel
)
{
    return internalPrep( pActivity, nodeId, fixedFlag, packLabel );
}


//  readDiskLabels()
//
//  To be called very early at boot time.  Builds a list of PackInfo objects.
//  This is used by BootActivity so that it can report the number of fixed devices to the console.
MFDManager::Result
MFDManager::readDiskLabels
(
    Activity* const         pActivity,
    COUNT* const            pFixedPacks
)
{
    Result result;
    lock();

    result = readDiskLabels( pActivity, &m_BootPackInfo );
    if ( result.m_Status == MFDST_SUCCESSFUL )
    {
        *pFixedPacks = 0;
        for ( CITPACKINFOLIST itpi = m_BootPackInfo.begin(); itpi != m_BootPackInfo.end(); ++itpi )
        {
            if ( (*itpi)->m_IsFixed )
                ++(*pFixedPacks);
        }
    }

    unlock();
    return result;
}


/*TODO:RECOV
//  recover()
//
//  Recovers the MFD manager.
MFDManager::Result
    MFDManager::recover()
{
    Result result;

    return result;
}
*/


//  releaseExclusiveUse()
//
//  Updates the main item for the file cycle to indicate that it is no longer assigned exclusively.
MFDManager::Result
MFDManager::releaseExclusiveUse
(
    Activity* const     pActivity,
    const DSADDR        mainItem0Addr
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::releaseExclusiveUse() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " mainItem0Addr=0" << std::oct << mainItem0Addr;
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    //  Read and update main item sector 0
    //  Main item 0[021].bit9 is exclusive use flag (clear it)
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, true );
        return result;
    }

    pMainItem0[021].setS2( pMainItem0[021].getS2() & 073 );

    result = commitMFDUpdates( pActivity );

    unlock();
    stopExecOnResultStatus( result, true );
    return result;
}


//  releaseFileCycle()
//
//  Updates the main item for the file cycle to indicate that it is no longer assigned
//  (or at least, less assigned than before).
//  For disk files, the loaded FAE assign count is decremented, and if zero, unloaded.
//  Note: does not do anything with releasing initial reserve - must be done by Facilities
MFDManager::Result
MFDManager::releaseFileCycle
(
    Activity* const     pActivity,
    const DSADDR        mainItem0Addr
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::releaseFileCycle() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " mainItem0Addr=0" << std::oct << mainItem0Addr;
        SystemLog::write( strm.str() );
    }

    Result result;
    lock();

    //  Read and update main item sector 0
    //  Main item 0[014].T1 is descriptor flags; bit5 (from the left) is to-be-cataloged (clear it)
    //  Main item 0[021].T2 is current assign count
    //  Main item 0[021].bit9 is exclusive use flag (clear it regardless)
    //  Main item 0[022].W is TDATE$ current assignment started or last assignment ended
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, true );
        return result;
    }

    //  Generate new assign count (the current count, minus one).
    //  If file is to-be-deleted and assign count goes to zero, we delete the cycle.
    //  File assign count is in word 17,T2, to-be-deleted is Word 12 bit 11.
    COUNT asgCount = pMainItem0[021].getT2();
    if ( asgCount == 0 )
    {
        result.m_Status = MFDST_INTERNAL_ERROR;
        unlock();
        std::stringstream strm;
        strm << "MFDManager::releaseFileCycle() assign count is zero for mainItem0Addr=0" << std::oct << mainItem0Addr;
        SystemLog::write( strm.str() );
        stopExecOnResultStatus( result, true );
        return result;
    }

    //  Update assign count, and set dropFlag if we need to drop the file cycle.
    COUNT newAsgCount = asgCount - 1;
    pMainItem0[021].setT2( newAsgCount );
    bool dropFlag = false;
    if ( (newAsgCount == 0) && ( (pMainItem0[014].getS2() & 01) == 01) )
        dropFlag = true;

    //  Clear to-be-cataloged and exclusive use flag (might already be cleared, but do it anyway)
    pMainItem0[014].logicalAnd( 0767777777777ll );
    pMainItem0[021].setS2( pMainItem0[021].getS2() & 073 );

    //  Update timestamp of last time file was asg'd/released
    TDate* pTDate = reinterpret_cast<TDate*>( &pMainItem0[022] );
    m_pExec->getExecTimeTDate( pTDate );

    //  If we are releasing a +1 cycle, adjust lead item accordingly
    DSADDR leadItem0Addr = getLinkAddress(pMainItem0[013]);
    Word36* pLeadItem0 = 0;
    if ( !stageDirectorySector( leadItem0Addr, false, &pLeadItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, true );
        return result;
    }

    if ( (pLeadItem0[012].getT1() & 02000) && (getLinkAddress(pLeadItem0[013]) == mainItem0Addr) )
    {
        //  Mark sector for update, and clear the +1 exists flag
        stageDirectorySector( leadItem0Addr, true, &pLeadItem0, &result );
        pLeadItem0[012].logicalAnd( 0500000000000ll );
    }

    //  For disk files, we (might) need to deal with the file allocation table...
    if ( newAsgCount == 0 )
    {
        DSADDR leadItem0Addr = getLinkAddress( pMainItem0[013] );
        Word36* pLeadItem0 = 0;
        if ( !stageDirectorySector( leadItem0Addr, false, &pLeadItem0, &result ) )
        {
            unlock();
            stopExecOnResultStatus( result, true );
            return result;
        }

        //TODO:REM need to deal with removable also
        if ( getFileType( pLeadItem0[011].getS1() ) == FILETYPE_MASS_STORAGE )
        {
            ITFILEALLOCATIONDICTIONARY itfad = m_FileAllocationDictionary.find( mainItem0Addr );
            if ( itfad == m_FileAllocationDictionary.end() )
            {
                result.m_Status = MFDST_INTERNAL_ERROR;
                unlock();
                std::stringstream strm;
                strm << "MFDManager::releaseFileCycle() FAT not in FAD for mainItem0Addr=0" << std::oct << mainItem0Addr;
                SystemLog::write( strm.str() );
                stopExecOnResultStatus( result, true );
                return result;
            }

            if ( itfad->second->isUpdated() )
            {
                itfad->second->synchronizeDADTables();
                result = writeDADUpdates( pActivity, itfad->second );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                {
                    unlock();
                    stopExecOnResultStatus( result, true );
                    return result;
                }
            }

            delete itfad->second;
            m_FileAllocationDictionary.erase( itfad );
        }
    }

    //  Do we need to drop the file cycle?
    //  If so, but it is on the print queue, do nothing (in this case, to-be-deleted is already set)
    //  If not on print queue, we can delete the thing now.
    if ( dropFlag )
    {
        //  Is the file on SMOQUE?  Check word 15,h1 - zero means not on queue.
        if ( pMainItem0[017].getH1() == 0 )
            dropFileCycle( pActivity, mainItem0Addr, false );
    }

    result = commitMFDUpdates( pActivity );

    unlock();
    stopExecOnResultStatus( result, true );
    return result;
}


//  releaseFileTracks()
//
//  Releases track allocations for the file indicated by the FAT in the given DiskFacilityItem.
//  The file is presumed to be assigned, as evidenced by the requirement of a DiskFacilityItem pointer.
//  For temporary files, we release allocations in the appropriate DiskAllocationTable as well
//  as updating the relevant FAT.  Temporary allocations are not tracked in the SMBT.
//  For cataloged files, we additionally update the FAT's DAD tables rewriting them to disk,
//  and release allocations in the appropriate SMBT.
MFDManager::Result
MFDManager::releaseFileTracks
(
    Activity* const         pActivity,
    DiskFacilityItem*       pDiskItem,
    const TRACK_ID          fileTrackId,
    const TRACK_COUNT       trackCount
)
{
    {//TODO:DEBUG
        std::stringstream strm;
        strm << "MFDManager::releaseFileTracks() "
            << "RUNID=" << pActivity->getRunInfo()->getActualRunId()
            << " FILE=" << pDiskItem->getQualifier() << "*" << pDiskItem->getFileName()
            << "(" << pDiskItem->getAbsoluteFileCycle() << ")"
            << " TrkId=" << fileTrackId
            << " TrkCount=" << trackCount;
        SystemLog::write( strm.str() );
    }

    Result result;

    //  Handle the trivial case
    if ( trackCount == 0 )
        return result;

    lock();

    //  Chunk up the allocation space so we don't try to release areas not allocated
    FileAllocationTable* pFAT = pDiskItem->getFileAllocationTable();
    FileAllocationTable::FAENTRIES faEntries;
    pFAT->getFileAllocationEntries( fileTrackId, trackCount, &faEntries );

    //  Iterate over the entries, taking action only for those which represent allocations
    for ( FileAllocationTable::CITFAENTRIES itfae = faEntries.begin(); itfae != faEntries.end(); ++itfae )
    {
        if ( itfae->second.m_LDATIndex != 0 )
        {
            //  Chop this bit out of the FAT first, so that if we die in the middle, we're less likely
            //  to end up with data corruption.
            if ( pFAT->released( itfae->first, itfae->second.m_TrackCount ) )
            {
                //  Now release the space in the DiskAllocationTable for the indicated pack
                result = deallocateFixedTracks( pActivity,
                                                itfae->second.m_LDATIndex,
                                                itfae->second.m_DeviceTrackId,
                                                itfae->second.m_TrackCount,
                                                pDiskItem->getTemporaryFileFlag() );
                if ( result.m_Status != MFDST_SUCCESSFUL )
                {
                    unlock();
                    stopExecOnResultStatus( result, false );
                    return result;
                }
            }
        }
    }

    //  Get the new value for highest granule assigned.  We do expect the caller to behave according
    //  to granularity, so in theory, all the numbers should be on granule boundaries.  But just in case...
    bool updateSector0 = false;
    COUNT tracksPerGranule = pDiskItem->getGranularity() == MSGRAN_POSITION ? 64 : 1;
    TRACK_COUNT newHighestTrackAssigned = pFAT->getHighestTrackAssigned();
    UINT64 newHighestGranuleAssigned = newHighestTrackAssigned / tracksPerGranule;
    if ( newHighestTrackAssigned % tracksPerGranule )
        ++newHighestGranuleAssigned;
    if ( newHighestGranuleAssigned < pDiskItem->getHighestGranuleAssigned() )
    {
        pDiskItem->setHighestGranuleAssigned( static_cast<UINT32>( newHighestGranuleAssigned ) );
        if ( newHighestTrackAssigned < pDiskItem->getHighestTrackWritten() )
            pDiskItem->setHighestTrackWritten( static_cast<UINT32>( newHighestTrackAssigned ) );
        updateSector0 = true;
    }

    //  If this is a cataloged file, synchronize the DAD tables in the FAT, then write them to disk.
    //  Also, if necessary, update main item sector 0.
    if ( !pDiskItem->getTemporaryFileFlag() )
    {
        pDiskItem->getFileAllocationTable()->synchronizeDADTables();
        if ( updateSector0 )
        {
            result = updateGranulesInfo( pActivity,
                                         pDiskItem->getMainItem0Addr(),
                                         pDiskItem->getInitialGranules(),
                                         pDiskItem->getMaximumGranules(),
                                         pDiskItem->getHighestGranuleAssigned(),
                                         pDiskItem->getHighestTrackWritten() );
            if ( result.m_Status != MFDST_SUCCESSFUL )
            {
                unlock();
                stopExecOnResultStatus( result, false );
                return result;
            }
        }
    }

    //TODO:REM needs attention here for updating removable MFD, if appropriate

    result = commitMFDUpdates( pActivity );
    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  setBadTrack()
//
//  Marks a track allocated in the HMBT and the SMBT.
MFDManager::Result
MFDManager::setBadTrack
(
    Activity* const   pActivity,
    const LDATINDEX   ldatIndex,
    const TRACK_ID    trackId
)
{
    lock();
    Result result = setSMBTAllocated( pActivity, ldatIndex, trackId, 1, true, true );
    unlock();

    stopExecOnResultStatus( result, false );
    return result;
}


//  shutdown()
//
//  Exec::ExecManager interface
//  Called during Exec stop, before terminating existing threads
void
MFDManager::shutdown()
{
    SystemLog::write( "MFDManager::shutdown()" );
}


//  startup()
//
//  Exec::ExecManager interface
//  Called during Exec boot, before BootThread starts
bool
MFDManager::startup()
{
    SystemLog::write( "MFDManager::startup()" );

    //  (re)load config data
    getConfigData();

    //  Clear out the cache
    m_DirectoryCache.clear();
    m_DirectoryTrackIdMap.clear();
    m_UpdatedSectors.clear();

    //  Get rid of other internal table cruft left from previous sessions.
    //  Do NOT get rid of stuff which should persist across sessions.
    while ( m_FileAllocationDictionary.size() > 0 )
    {
        ITFILEALLOCATIONDICTIONARY itfad = m_FileAllocationDictionary.begin();
        delete itfad->second;
        m_FileAllocationDictionary.erase( itfad );
    }

    m_SearchItemLookupTable.clear();
    m_SearchItemLookupTable.resize( m_LookupTableSize, 0 );

    return true;
}


//  terminate()
//
//  Exec::ExecManager interface
//  Will be called during Exec stop, after existing threads terminate
void
MFDManager::terminate()
{
    SystemLog::write( "MFDManager::terminate()" );
}


//  updateFileCycle()
//
//  Updates information in the main item sector 0 for a file cycle.
//  Called from Facilities under certain @ASG conditions
MFDManager::Result
MFDManager::updateFileCycle
(
    Activity* const     pActivity,
    const DSADDR        mainItem0Addr,
    const bool          exclusiveFlag,
    const COUNT32       newInitialReserve,
    const COUNT32       newMaxGranules
)
{
    Result result;
    lock();

    //  Read and update main item sector 0
    //  Main item 0[021].bit9 is exclusive use flag
    //  Main item 0[024].H1 is initial reserve
    //  Main item 0[025].H1 is max granules
    Word36* pMainItem0 = 0;
    if ( !stageDirectorySector( mainItem0Addr, true, &pMainItem0, &result ) )
    {
        unlock();
        stopExecOnResultStatus( result, false );
        return result;
    }

    if ( exclusiveFlag )
        pMainItem0[021].setS2( pMainItem0[021].getS2() | 04 );
    pMainItem0[024].setH1( newInitialReserve );
    pMainItem0[025].setH1( newMaxGranules );

    result = commitMFDUpdates( pActivity );
    unlock();
    stopExecOnResultStatus( result, false );
    return result;
}


//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------
//  public static methods
//  ------------------------------------------------------------------------------------------------
//  ------------------------------------------------------------------------------------------------

//  getFileType()
//
//  Converts a numeric fileType from and MFD entry to a FileType enumerator.
MFDManager::FileType
MFDManager::getFileType
(
    const UINT8             mfdFileType
)
{
    switch ( mfdFileType )
    {
    case 0:     return FILETYPE_MASS_STORAGE;
    case 01:    return FILETYPE_TAPE;
    case 040:   return FILETYPE_REMOVABLE;
    }
    return FILETYPE_NONE;
}


//  getFileTypeString()
//
//  converts a FileType enumeration to displayable text
std::string
MFDManager::getFileTypeString
(
    const FileType      fileType
)
{
    switch ( fileType )
    {
    case FILETYPE_NONE:             return "None";
    case FILETYPE_MASS_STORAGE:     return "Mass Storage";
    case FILETYPE_TAPE:             return "Tape";
    case FILETYPE_REMOVABLE:        return "Removable";
    }

    return "???";
}


//  getResultString()
//
//  converts an MFDManager::Result object to displayable text
std::string
MFDManager::getResultString
(
    const Result&           result
)
{
    std::string str = getStatusString( result.m_Status );

    if ( result.m_Status == MFDST_IO_ERROR )
    {
        str += ":";
        str += ChannelModule::getStatusString( result.m_ChannelStatus );
        if ( result.m_ChannelStatus == ChannelModule::Status::DEVICE_ERROR )
            str += ":" + Device::getIoStatusString( result.m_DeviceStatus, result.m_SystemErrorCode );
    }

    return str;
}


//  getStatusString()
//
//  converts an MFDManager::Status object to displayable text
std::string
MFDManager::getStatusString
(
    const Status            status
)
{
    switch ( status )
    {
    case MFDST_SUCCESSFUL:              return "Successful";
    case MFDST_DEVICE_NOT_ACCESSIBLE:   return "Device Not Accessible";
    case MFDST_FILE_ON_PRINT_QUEUE:     return "File on print queue";
    case MFDST_FRESHLY_PREPPED:         return "Freshly Prepped";
    case MFDST_INTERNAL_ERROR:          return "Internal Error";
    case MFDST_IO_ERROR:                return "IO Error";
    case MFDST_LDAT_CONFLICT:           return "LDAT Conflict";
    case MFDST_MAX_GRANULES_EXCEEDED:   return "Max Granules Exceeded";
    case MFDST_NO_PATH:                 return "No Path Available";
    case MFDST_NOT_FOUND:               return "Not Found";
    case MFDST_OPERATOR_ABORTED_ACTION: return "Operator Aborted Action";
    case MFDST_OUT_OF_SPACE:            return "Out of Space";
    case MFDST_PACK_NAME_CONFLICT:      return "Pack Name Conflict";
    case MFDST_PACK_NOT_FORMATTED:      return "Pack Not Formatted";
    case MFDST_PACK_NOT_PREPPED:        return "Pack Not Prepped";
    case MFDST_SECTOR1_CONFLICT:        return "Sector1 Conflict";
    case MFDST_TERMINATING:             return "OS is terminating";
    case MFDST_UNSUPPORTED_PACK:        return "Unsupported Pack";
    }

    return "???";
}



















#if 0//TODO:RECOV recovery functions which we might use later
//  private methods

//  recoverFileDisposition()
//
//  Drop all file cycles which are in to-be state.
//      To-be-cataloged files resulted from @ASG,C or @ASG,U, which means they were to be cataloged when the
//      run terminated.  Since the system stopped, the run never actually terminated - thus, the files should
//      not be cataloged.
//      To-be-deleted files are to be dropped when the assign count goes to zero.  A system stop suffices.
//      However, do not drop any file cycles which are on SMOQUE.
//
//  For file cycles which were assigned during the stop, and had been updated, set them as software-disabled.
bool
    MFDManager::recoverFileDisposition()
{
    return true;
}


//  recoverTrackAllocationLead()
//
//  Given the DSADDR of a lead item, we check to ensure it is a mass storage file, and then iterate over the
//  attached main items, updating the track allocator based on the DADs for each main item.
//
//  Only return false if there's an error in recovery.
//  If the leadItemAddr is not for a mass-storage file, this is NOT an error, we just don't do anything.
bool
    MFDManager::recoverTrackAllocationLead
    (
    const DSADDR            leadItem0Addr
    )
{
    //  Grab lead item 0, to see if this is a mass storage file.  If it isn't, exit quietly.
    Word36* pLeadItem0 = 0;
    if ( !stageSector( leadItem0Addr, true, false, &pLeadItem0 ) )
    {
        SystemLog::write( "MFDManager::recoverTrackAllocation cannot read lead item 0" );
        return false;
    }

    if ( !isMassStorageFile( pLeadItem0 ) )
        return true;

    //  If there is a lead item 1, read it also.
    Word36* pLeadItem1 = 0;
    if ( isLinkedItem( pLeadItem0 ) )
    {
        DSADDR dsAddr1 = getLinkDSAddress( &pLeadItem0[0] );
        if ( !stageSector( dsAddr1, true, false, &pLeadItem1 ) )
        {
            SystemLog::write( "MFDManager::recoverTrackAllocation cannot read lead item 1" );
            return false;
        }
    }

    //  Create a cycle set info object.
    CycleSetInfo csInfo;
    getCycleSetInfo( pLeadItem0, pLeadItem1, &csInfo );

    //  Iterate over the existing cycles, recovering track allocation for each individual main item.
    for ( INDEX cx = 0; cx < csInfo.m_Cycles.size(); ++cx )
    {
        if ( csInfo.m_Cycles[cx].m_Exists )
            recoverTrackAllocationMain( csInfo.m_Cycles[cx].m_MainItem0Addr );
    }

    return true;
}


//  recoverTrackAllocationMain()
//
//  Given the DSADDR of a main item, iterate over that file cycle's DAD tables to update track allocation.
//
//  Only return false if there's an error in recovery.
bool
    MFDManager::recoverTrackAllocationMain
    (
    const DSADDR            mainItem0Addr
    )
{
    //  Grab main item sector 0.
    Word36* pMainItem0 = 0;
    if ( !stageSector( mainItem0Addr, true, false, &pMainItem0 ) )
    {
        SystemLog::write( "MFDManager:recoverTrackAllocation cannot read main item 0" );
        return false;
    }

    //  If there are no DAD items, exit quietly.
    if ( !isLinkedItem( pMainItem0 ) )
        return true;

    //  Now follow the chain of DAD items.
    DSADDR dadItemAddr = getLinkDSAddress( pMainItem0 );
    while ( dadItemAddr != 0 )
    {
        //  Stage the (next) DAD sector
        Word36* pDAD = 0;
        if ( !stageSector( dadItemAddr, false, false, &pDAD ) )
            return false;

        //  We have a DAD table.  This is a little messy, but not so much that you won't sleep at night.
        //  Word 0 is directory address of next DAD table if any.  0400000000000 means no more exist.
        //  Word 1 is a backward link to the previous DAD, or to the main item
        //  Word 2 is the file-relative address of the first word accounted for in this table
        //  Word 3 is the file-relative address of the last word accounted for in this table, plus 1.
        //  Word 4-6 is the first DAD entry, of which there are up to eight.
        //      DAD word 0 is the device-relative address of the 1st word accounted for by this DAD
        //          this should always be on a track boundary (divisible by 1792)
        //      DAD word 1 is the number of contiguous words accounted for by this DAD
        //      DAD word 2:H1 is DAD flags
        //          Bit 15: This is the last DAD entry in this table (the only bit that should be set, for us)
        //      DAD word 2:H2 is Device Index or 0400000 for a sparse entry.  For us, the index is always 0.
        for ( INDEX dx = 0; dx < 8; ++dx )
        {
            INDEX wx = 4 + dx * 3;
            if ( pDAD[wx + 2].getH2() != 0400000 )
            {
                //  Force-allocate the represented tracks.
                TRACKCOUNT tracks = static_cast<COUNT>(pDAD[wx + 1].getW() / 1792);
                TRACKID trackId = static_cast<TRACKID>(pDAD[wx].getW() / 1792);
                if ( !allocateTracks( trackId, tracks ) )
                {
                    //     This is a BAD thing - we should mark the file disabled, but we also need
                    //      to mark the conflicting file disabled as well, which is quite more difficult
                    std::string msg = "MFD DAD table inconsistency";
                    SystemLog::write( msg );
                    m_pConsoleManager->postReadOnlyMessage( msg, m_GenericRouting );
                }
            }

            if ( pDAD[wx + 2].getH1() & 04 )
                break;
        }

        //  Is there another DAD sector?
        if ( !isLinkedItem( pDAD ) )
            break;
        dadItemAddr = getLinkDSAddress( pDAD );
    }

    return true;
}


//  recoverTrackAllocationTable()
//
//  Go through the main items for disk files (only), following the chains for all of the DAD's,
//  and update the track allocation table accordingly.
//  Since we are restricting this to disk file, we must start with the lead item.
//
//  If we ever decide to use MBT's, then the whole track allocation table loading thing
//  can be done strictly from the SMBT.
bool
    MFDManager::recoverTrackAllocationTable()
{
    //  Iterate over the search items in order to find lead item DSADDRs.
    for ( INDEX hx = 0; hx < m_LookupTableSize; ++hx )
    {
        SEARCHENTRYLIST searchEntries;
        getSearchEntryList( hx, &searchEntries );
        for ( CITSEARCHENTRYLIST it = searchEntries.begin(); it != searchEntries.end(); ++it )
            recoverTrackAllocationLead( it->m_LeadItemAddress );
    }

    return true;
}





//  public methods

//  recover()
//
//  Recovers mass storage after a system reboot.
//  Single-thread through here.
MFDManager::Status
    MFDManager::recover()
{
    lock();

    //  Clear internal tables and objects, and reload configuration data
    clear();
    getConfigData();

    //  Open the cache.
    if ( !openCache( m_DataStoreName ) )
    {
        unlock();

        std::string logMsg = "MFDManager::recover cannot open datastore '";
        logMsg += m_DataStoreName;
        logMsg += "'";
        SystemLog::write( logMsg );

        m_pConsoleManager->postReadOnlyMessage( logMsg, m_GenericRouting );
        return MFDST_DATASTORE_ERROR;
    }

    //  Build the directory track lookup table
    if ( !recoverDirectoryTrackLookupTable() )
    {
        unlock();
        return MFDST_CONSISTENCY_ERROR;
    }

    //  Size and load the directory lookup table
    if ( !recoverLookupTable( m_LookupTableSize ) )
    {
        unlock();
        return MFDST_CONSISTENCY_ERROR;
    }

    //  Load track allocation table
    if ( !recoverTrackAllocationTable() )
    {
        unlock();
        return MFDST_CONSISTENCY_ERROR;
    }

    //  Handle file cycles in to-be state - basically, delete them all (except those on SMOQUE).
    if ( !recoverFileDisposition() )
    {
        unlock();
        return MFDST_CONSISTENCY_ERROR;
    }

    //  Fix files which were assigned at the time the system stopped
    if ( !recoverFixAssignedCycles() )
        return false;

    //  Delete files which are in to-be-catalogued status, since they should only finally be
    //  catalogued if the run finished successfully (or at all) -- which didn't happen, as we're rebooting.
    //  Also delete files which are in to-be-deleted status, since the deletion is guaranteed to occur
    //  once the files are no longer assigned anywhere, which will also be the case here
    //  (except for files on SMOQUE)
    if ( !recoverDropPendingCycles() )
        return false;

    //  Done
    unlock();
	std::string str = "MFDManager::recover() completed";
	SystemLog::write( str );

    return MFDST_SUCCESSFUL;
}



//  recoverFixAssignedCycles()
//
//  Find all files which were assigned when the previous session was stopped; mark them unassigned.
//      Sector 0 Word 17:T2 is the assign count - if non-zero, the file was assigned
//  For (previously-assigned) disk files which were writen to at the time, mark them software-disabled.
//      Sector 0 Word 12:T2 File Flags bit 16 means the file was written to
//  Disable flags are in Sector 0 Word 11
//      Bit 0: set for any condition
//      Bit 2: set for software disable
bool
    MFDManager::recoverFixAssignedCycles()
{
    for ( ITDSIMAP it = m_DirectorySectors.begin(); it != m_DirectorySectors.end(); ++it )
    {
        if ( isMainItem( it ) && (it->second.m_Data[17].getT2() > 0) )
        {
            it->second.m_Data[17].setT2( 0 );
            it->second.m_Dirty = true;

            DSADDR leadItemAddr = it->second.m_Data[11].getW() & 07777777777;
            CITDSIMAP itLeadItem;
            if ( getDirectorySectorInfoIterator( leadItemAddr, &itLeadItem ) )
            {
                if ( isDiskFile( itLeadItem ) && ( it->second.m_Data[12].getT2() == 0200 ) )
                {
                    it->second.m_Data[11].logicalOr( 0500000000000LL );
                    it->second.m_Dirty = true;
                }
            }
        }
    }

    return writeUpdatedDirectorySectors();
}


//  private, protected static methods

#endif