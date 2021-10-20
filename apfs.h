#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "dmgParser.h"

/* From: https://www.ntfs.com/apfs-structure.htm */

#define MAX_CKSUM_SIZE 8

#define APFS_MAX_HIST 8
#define APFS_VOLNAME_LEN 256
#define APFS_MODIFIED_NAMELEN 32


typedef uint8_t  tApFS_Uuid;
typedef uint64_t tApFS_Ident;
typedef uint64_t tApFS_Transaction;
typedef uint64_t  tApFS_Address;
typedef uint64_t tApFS_BTreeKey;

typedef uint64_t oid_t;
typedef uint64_t xid_t;

typedef uint32_t cp_key_class_t;
typedef uint32_t cp_key_os_version_t;
typedef uint16_t cp_key_revision_t;
typedef uint32_t crypto_flags_t;
typedef unsigned char uuid_t[16];



typedef struct apfs_block_header {
	uint64_t checksum;
	uint64_t block_id;
	uint64_t version;
	uint16_t block_type;
	uint16_t flags;
	uint32_t padding;
} APFS_BH;

typedef struct tApFS_BlockRange
{
    tApFS_Address       First;          // First block
    uint64_t              Count;          // Blocks' amount
}tApFS_BlockRange;

struct tApFS_COH
{
    uint64_t              CheckSum;       // Control object sum
    tApFS_Ident         Ident;          // Object identifier
    tApFS_Transaction   Transaction;    // Object change transaction number
    uint16_t              Type;           // Object type
    uint16_t              Flags;          // Object flags
    uint32_t              SubType;        // Object subType
};

enum eApFS_ObjectType
{
    eApFS_ObjectType_01_SuperBlock            = 0x0001, // Container Superblock
    eApFS_ObjectType_02_BTreeRoot             = 0x0002, // B-Tree: root node
    eApFS_ObjectType_03_BTreeNode             = 0x0003, // B-Tree: non-root node
    eApFS_ObjectType_05_SpaceManager          = 0x0005, // _Space Manager_
    eApFS_ObjectType_06_SpaceManagerCAB       = 0x0006, // _Space Manager_ segments' addresses info
    eApFS_ObjectType_07_SpaceManagerCIB       = 0x0007, // _Space Manager_ segments' info
    eApFS_ObjectType_08_SpaceManagerBitmap    = 0x0008, // Free space bitmap used by _Space Manager_
    eApFS_ObjectType_09_SpaceManagerFreeQueue = 0x0009, // Free space queue used by _Space Manager_ (keys - _tApFS_09_SpaceManagerFreeQueue_Key_, values - _tApFS_09_SpaceManagerFreeQueue_Value_)
    eApFS_ObjectType_0A_ExtentListTree        = 0x000A, // Extents' List Tree (keys - offset beginning extent _tApFS_Address_, values - physical data location _tApFS_BlockRange_)
    eApFS_ObjectType_0B_ObjectsMap            = 0x000B, // Type - Objects Map; subType - Object Map record tree (keys - _tApFS_0B_ObjectsMap_Key_, values - _tApFS_0B_ObjectsMap_Value_)
    eApFS_ObjectType_0C_CheckPointMap         = 0x000C, // Check Point Map
    eApFS_ObjectType_0D_FileSystem            = 0x000D, // Volume file system
    eApFS_ObjectType_0E_FileSystemTree        = 0x000E, // File system tree (keys start from _tApFS_BTreeKey_, describes key type and value)
    eApFS_ObjectType_0F_BlockReferenceTree    = 0x000F, // Block Reference Tree (keys - _tApFS_BTreeKey_, values - _tApFS_0F_BlockReferenceTree_Value_)
    eApFS_ObjectType_10_SnapshotMetaTree      = 0x0010, // Snapshot Meta Tree (keys - _tApFS_BTreeKey_, values - _tApFS_10_SnapshotMetaTree_Value_)
    eApFS_ObjectType_11_Reaper                = 0x0011, // Reaper
    eApFS_ObjectType_12_ReaperList            = 0x0012, // Reaper List
    eApFS_ObjectType_13_ObjectsMapSnapshot    = 0x0013, // Objects Map Snapshot tree (keys - _tApFS_Transaction_, values - _tApFS_13_ObjectsMapSnapshot_Value_)
    eApFS_ObjectType_14_JumpStartEFI          = 0x0014, // EFI Loader
    eApFS_ObjectType_15_FusionMiddleTree      = 0x0015, // Fusion devices tree to track SSD-cached HDD blocks (keys - _tApFS_Address_, values - _tApFS_15_FusionMiddleTree_Value_)
    eApFS_ObjectType_16_FusionWriteBack       = 0x0016, // Fusion devices writeback cache status
    eApFS_ObjectType_17_FusionWriteBackList   = 0x0017, // Fusion devices writeback cache list
    eApFS_ObjectType_18_EncryptionState       = 0x0018, // Encryption
    eApFS_ObjectType_19_GeneralBitmap         = 0x0019, // General Bitmap
    eApFS_ObjectType_1A_GeneralBitmapTree     = 0x001A, // General Bitmap Tree (keys - uint64_t, keys - uint64)
    eApFS_ObjectType_1B_GeneralBitmapBlock    = 0x001B, // General Bitmap Block
    eApFS_ObjectType_00_Invalid               = 0x0000, // Non-valid as a type or absent subType
    eApFS_ObjectType_FF_Test                  = 0x00FF  // Reserved for testing (never stored on media)
};

enum eApFS_ObjectFlag
{
    eApFS_ObjectFlag_Virtual         = 0x0000, // Virtual object
    eApFS_ObjectFlag_Ephemeral       = 0x8000, // Ephemeral object
    eApFS_ObjectFlag_Physical        = 0x4000, // Physical object
    eApFS_ObjectFlag_NoHeader        = 0x2000, // Object with no _tApFS_ContainerObjectHeader_ header (for example, a Space Manager bitmap)
    eApFS_ObjectFlag_Encrypted       = 0x1000, // Encrypted object
    eApFS_ObjectFlag_NonPersistent   = 0x0800, // Object with this flag is never saved on the media
    eApFS_ObjectFlag_StorageTypeMask = 0xC000, // Bitmask for accessing object category flags
    eApFS_ObjectFlag_ValidMask       = 0xF800  // Valid flag bitmask
};

struct tApFS_0B_ObjectsMap_Key
{
    tApFS_Ident       ObjectIdent; // Object Identifier
    tApFS_Transaction Transaction; // Transaction number
};
typedef struct tApFS_0B_ObjectsMap_Key tApFS_0B_ObjectsMap_Key_t;

struct tApFS_0B_ObjectsMap_Value
{
    uint32_t       Flags;    // Flags
    uint32_t       Size;     // Object size in bytes (a multiple of the container block size)
    tApFS_Address Address; // Object address
};
typedef struct tApFS_0B_ObjectsMap_Value tApFS_0B_ObjectsMap_Value_t;

struct tApFS_09_SpaceManagerFreeQueue_Key
{
    tApFS_Transaction sfqk_xid;
    tApFS_Address     sfqk_paddr;
};

struct tApFS_09_SpaceManagerFreeQueue_Value
{
    uint64_t            sfq_count;
    tApFS_Ident       sfq_tree_oid;
    tApFS_Transaction sfq_oldest_xid;
    uint16_t            sfq_tree_node_limit;
    uint16_t            sfq_pad16;
    uint32_t            sfq_pad32;
    uint64_t            sfq_reserved;
};

struct tApFS_10_SnapshotMetaTree_Value
{
    tApFS_Ident ExtentRefIdent;    // Identifier of the B-tree physical object that stores the extent information
    tApFS_Ident SuperBlockIdent;   // Superblock identifier
    uint64_t      CreatedTime;       // Snapshot creation time (in nanoseconds from midnight 01/01/1970)
    uint64_t      LastModifiedTime;  // Snapshot last modified time (in nanoseconds from midnight 01/01/1970)
    uint64_t      iNum;              
    uint32_t      ExtentRefTreeType; // Type of B-tree that stores extent information
    uint16_t      NameLength;        // Snapshot name length (including end of line character)
    uint8_t       Name[];            // Snapshot name (ending with 0)
};

struct tApFS_13_ObjectsMapSnapshot_Value
{
    uint32_t      Flags;    // Snapshot flags
    uint32_t      Padding;  // Reserved (for adjustment)
    tApFS_Ident Reserved; // Reserved
};

struct tApFS_15_FusionMiddleTree_Value
{
    tApFS_Address fmv_lba;
    uint32_t        fmv_length;
    uint32_t        fmv_flags;
};

typedef struct apfs_super_block {
	uint32_t 		MagicNumber 		/* (NXSB) 	A value that can be used to verify that we are reading an instance of CS */;
	uint32_t 		BlockSize 		/* Container block size (in bytes) */;
	uint64_t 		BlocksCount 		/* Container blocks' amount */;
	uint64_t 		Features 		/* Container basic features' bitmap */;
	uint64_t 		ReadOnlyFeatures 	/* Container basic read-only features' bitmap */;
	uint64_t 		IncompatibleFeatures 	/* Container basic incompatible features' bitmap */;
	tApFS_Uuid 		Uuid[16] 			/* Container's UUID */;
	tApFS_Ident 		NextIdent 		/* Next identifier for new ephemeral or virtual object */;
	tApFS_Transaction 	NextTransaction 	/* Next transaction number */;
	uint32_t 		DescriptorBlocks 	/* Amount of blocks used by descriptor */;
	uint32_t 		DataBlocks 		/* Amount of blocks used by data */;
	tApFS_Address 		DescriptorBase 		/* Descriptor base address or physical object Id with address tree */;
	int 			DataBase 		/* Data base address or physical object Id with address tree */;
	uint32_t 		DescriptorNext 		/* Next descriptor index */;
	uint32_t 		DataNext 		/* Next data index */;
	uint32_t 		DescriptorIndex 	/* Index of first valid element in descriptor segment */;
	uint32_t 		DescriptorLength 	/* Blocks' amount in descriptor segment used by superblock */;
	uint32_t 		DataIndex 		/* Index of first valid element in data segment */;
	uint32_t 		DataLength 		/* Blocks' amount in data segment used by superblock */;
	tApFS_Ident 		SpaceManagerIdent 	/* Space Manager ephemeral object identifier */;
	tApFS_Ident 		ObjectsMapIdent 	/* Physical object identifier of the container object map */;
	tApFS_Ident 		ReaperIdent 		/* Reaper Ephemeral Object Identifier */;
	uint32_t 		ReservedForTesting0; 	
	uint32_t 		MaximumVolumes 		/* Maximum possible number of volumes per container */;
	tApFS_Ident 		VolumesIdents[100] 	/* Volume array of virtual object identifiers */;
	uint64_t 		Counters[32] 		/* Array of counters storing container information */;
	tApFS_BlockRange 	BlockedOutOfRange 	/* Physical range of blocks that cannot be used */;
	tApFS_Ident 		MappingTreeIdent 	/* Physical object identifier of the tree used to track objects to be moved from locked storage */;
	uint64_t 		OtherFlags 		/* Other container functions' bitmap */;
	tApFS_Address 		JumpstartEFI 		/* Physical object Id with EFI-driver data */;
	tApFS_Uuid 		FusionUuid 		/* Fusion container UUID or zero for non-Fusion containers */;
	tApFS_BlockRange 	KeyLocker 		/* Container Key Tag Location */;
	uint64_t 		EphemeralInfo[4] 	/* Fields' array used to manage ephemeral data */;
	tApFS_Ident 		ReservedForTesting1;
	tApFS_Ident 		FusionMidleTreeIdent 	/* Fusion devices only */;
	tApFS_Ident 		FusionWriteBackIdent 	/* Fusion devices only */;
	tApFS_BlockRange 	FusionWriteBackBlocks;
} APFS_SuperBlk;

// Omap Header Structure
struct obj_phys {
uint8_t o_cksum[MAX_CKSUM_SIZE];
oid_t o_oid;
xid_t o_xid;
uint32_t o_type;
uint32_t o_subtype;
};
typedef struct obj_phys obj_phys_t;


struct checkpoint_mapping {
uint32_t cpm_type;
uint32_t cpm_subtype;
uint32_t cpm_size;
uint32_t cpm_pad;
oid_t cpm_fs_oid;
oid_t cpm_oid;
oid_t cpm_paddr;
};
typedef struct checkpoint_mapping checkpoint_mapping_t;

typedef struct checkpoint_map_phys {
obj_phys_t cpm_o;
uint32_t cpm_flags;
uint32_t cpm_count;
checkpoint_mapping_t cpm_map[];
}checkPoint_Map;

struct omap_phys {
obj_phys_t om_o;
uint32_t om_flags;
uint32_t om_snap_count;
uint32_t om_tree_type;
uint32_t om_snapshot_tree_type;
oid_t om_tree_oid;
oid_t om_snapshot_tree_oid;
xid_t om_most_recent_snap;
xid_t om_pending_revert_min;
xid_t om_pending_revert_max;
};
typedef struct omap_phys omap_phys_t;

struct nloc {
uint16_t off;
uint16_t len;
};
typedef struct nloc nloc_t;

struct kvloc {
nloc_t k;
nloc_t v;
};
typedef struct kvloc kvloc_t;

struct kvoff {
uint16_t k;
uint16_t v;
};
typedef struct kvoff kvoff_t;



struct btree_node_phys {
obj_phys_t btn_o;
uint16_t btn_flags;
uint16_t btn_level;
uint32_t btn_nkeys;
nloc_t btn_table_space;
nloc_t btn_free_space;
nloc_t btn_key_free_list;
nloc_t btn_val_free_list;
uint64_t btn_data[];
};
typedef struct btree_node_phys btree_node_phys_t;

struct btree_info_fixed {
uint32_t bt_flags;
uint32_t bt_node_size;
uint32_t bt_key_size;
uint32_t bt_val_size;
};
typedef struct btree_info_fixed btree_info_fixed_t;

struct btree_info {
btree_info_fixed_t bt_fixed;
uint32_t bt_longest_key;
uint32_t bt_longest_val;
uint64_t bt_key_count;
uint64_t bt_node_count;
};
typedef struct btree_info btree_info_t;

struct wrapped_meta_crypto_state {
uint16_t major_version;
uint16_t minor_version;
crypto_flags_t cpflags;
cp_key_class_t persistent_class;
cp_key_os_version_t key_os_version;
cp_key_revision_t key_revision;
uint16_t unused;
} __attribute__((aligned(2), packed));
typedef struct wrapped_meta_crypto_state wrapped_meta_crypto_state_t;


struct apfs_modified_by {
uint8_t id[APFS_MODIFIED_NAMELEN];
uint64_t timestamp;
xid_t last_xid;
};
typedef struct apfs_modified_by apfs_modified_by_t;


struct apfs_superblock {
obj_phys_t apfs_o;
uint32_t apfs_magic;
uint32_t apfs_fs_index;
uint64_t apfs_features;
uint64_t apfs_readonly_compatible_features;
uint64_t apfs_incompatible_features;
uint64_t apfs_unmount_time;
uint64_t apfs_fs_reserve_block_count;
uint64_t apfs_fs_quota_block_count;
uint64_t apfs_fs_alloc_count;
wrapped_meta_crypto_state_t apfs_meta_crypto;
uint32_t apfs_root_tree_type;
uint32_t apfs_extentref_tree_type;
uint32_t apfs_snap_meta_tree_type;
oid_t apfs_omap_oid;
oid_t apfs_root_tree_oid;
oid_t apfs_extentref_tree_oid;
oid_t apfs_snap_meta_tree_oid;
xid_t apfs_revert_to_xid;
oid_t apfs_revert_to_sblock_oid;
uint64_t apfs_next_obj_id;
uint64_t apfs_num_files;
uint64_t apfs_num_directories;
uint64_t apfs_num_symlinks;
uint64_t apfs_num_other_fsobjects;
uint64_t apfs_num_snapshots;
uint64_t apfs_total_blocks_alloced;
uint64_t apfs_total_blocks_freed;
uuid_t apfs_vol_uuid;
uint64_t apfs_last_mod_time;
uint64_t apfs_fs_flags;
apfs_modified_by_t apfs_formatted_by;
apfs_modified_by_t apfs_modified_by[APFS_MAX_HIST];
uint8_t apfs_volname[APFS_VOLNAME_LEN];
uint32_t apfs_next_doc_id;
uint16_t apfs_role;
uint16_t reserved;
xid_t apfs_root_to_xid;
oid_t apfs_er_state_oid;
uint64_t apfs_cloneinfo_id_epoch;
uint64_t apfs_cloneinfo_xid;
oid_t apfs_snap_meta_ext_oid;
uuid_t apfs_volume_group_id;
oid_t apfs_integrity_meta_oid;
oid_t apfs_fext_tree_oid;
uint32_t apfs_fext_tree_type;
uint32_t reserved_type;
oid_t reserved_oid;
};
typedef struct apfs_superblock apfs_superblock_t;

void parse_APFS( int );	
APFS_SuperBlk findValidSuperBlock( FILE * );
omap_phys_t parseValidContainerSuperBlock( FILE *,APFS_SuperBlk );
apfs_superblock_t findValidVolumeSuperBlock( FILE *,omap_phys_t,APFS_SuperBlk );

