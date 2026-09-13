/* NuMC3DS seam table.
 *
 * Subsystems: alloc, string, nbt, level, weather, block, item, inventory,
 *             entity, selector, player, mob, network, ui, screen, world,
 *             factory, spawn, enchant, gamerule, teleport, sound, event,
 *             misc, arm
 */
#ifndef NUMC3DS_SEAMS_H
#define NUMC3DS_SEAMS_H

/* == allocator / heap == */
#define SEAM_Heap_allocWithSelector   0x0011493Cu  /* local_static_verified */
#define SEAM_Heap_alloc               0x0011494Cu  /* from FOUNDATION-INDEX */
#define SEAM_Heap_free                0x001146E8u  /* from FOUNDATION-INDEX */
#define SEAM_gstd_allocator_allocate  0x001010CBu
#define SEAM_gstd_allocator_deallocate 0x001007D1u
#define SEAM_operator_new             0x002FF4EDu
#define SEAM_operator_delete          0x002FF2F0u
#define SEAM_game_alloc_selector      0x00994898u  /* data: allocation descriptor */

/* == gstd string == */
#define SEAM_StrCtor                  0x002FF221u
#define SEAM_StrDtor                  0x002FEBBDu
#define SEAM_StrAssign                0x002FFFD9u
#define SEAM_gstd_string_compare       0x00137850u
#define SEAM_gstd_string_init         0x002FF221u /* gstd_string_init is Thumb; callers reach it via blx rN, so bit0 must be set */
#define SEAM_gstd_string_allocate     0x002FEB29u /* gstd_string_allocate is Thumb; even address would decode as ARM */
#define SEAM_gstd_string_copyCtor     0x002FF261u /* gstd_string_copyCtor is Thumb (ARM thunk at 0x002FF258 does adr r12,0x2FF261; bx r12) */
#define SEAM_gstd_memcmp              0x00101B44u
#define SEAM_SignedDivide             0x003021F0u
#define SEAM_aeabi_uidivmod           0x002FFF64u
#define SEAM_StringUtils_hashCode     0x0012A814u
#define SEAM_Localization_get         0x0011D954u
#define SEAM_gstd_snprintfBuffered    0x00100B89u

/* == CTR filesystem / SDMC ==
 * These are exact USA 1.9.19 seams.  The stock FileManager path resolver
 * deliberately knows only the internal media prefixes; world_transfer uses
 * the lower-level archive registration path to add the SDK's "sdmc:" alias
 * without changing the canonical extdata save paths. */
#define SEAM_nn_fs_RegisterArchive             0x00122868u /* local_static_verified */
#define SEAM_nn_fs_OpenSpecialArchiveRaw      0x00122BB4u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_FindArchiveWide            0x001335FCu /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_FindArchiveChar            0x007E33B4u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_SetSdmcEjectionFatal       0x004AB6CCu /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_TryReadFile                0x0011B060u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_TryWriteFile               0x004AC684u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_TryReadDirectory           0x004AC890u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_FileClose                  0x004ACC38u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_DirectoryClose             0x004ACEE8u /* library_bsim_single_variant_verified */
#define SEAM_nn_fs_FileManagerTryInitializeFile 0x001226FCu /* local_static_verified */
#define SEAM_nn_fs_FileManagerTryInitializeDirectory 0x00132844u /* local_static_verified */
#define SEAM_nn_fs_FileManagerTryCreateFile  0x004AC6E8u /* local_static_verified */
#define SEAM_nn_fs_FileManagerTryCreateDirectory 0x001328D0u /* local_static_verified */
#define SEAM_nn_fs_FileManagerTryRenameFile  0x004AC7D0u /* local_static_verified */
#define SEAM_nn_fs_DeleteArchiveFilePath     0x001288B8u /* local_static_verified */
#define SEAM_ExternalFileLevelStorage_readLevelDataFromFile 0x004738A8u /* local_static_verified */
#define SEAM_ExternalFileLevelStorageSource_getLevelListByMap 0x004CBBF8u /* reference_correlated_static_verified */
#define SEAM_FileManager_allocateFileStreamFromPath 0x00119B50u /* local_static_verified */
#define SEAM_File_getPathSize 0x0055B4B8u /* local_static_verified */
#define SEAM_FixedSlotStorage_writeSaveMarker 0x00319A08u /* local_static_verified */
#define SEAM_nn_fs_TryGetSize 0x0011B0C0u /* local_static_verified */
#define SEAM_ResourceFile_open 0x0010C86Cu
#define SEAM_ResourceFile_read 0x0010C958u
#define SEAM_ResourceFile_getSize 0x0010C9D8u
#define SEAM_ResourceFile_constructor 0x0010CA24u
#define SEAM_ResourceFile_destructor 0x0010CA5Cu
#define SEAM_LevelSummary_copy 0x001D9230u /* local_static_verified */
#define SEAM_LevelSummary_dtor 0x001D92C0u /* local_static_verified */
#define SEAM_vector_LevelSummary_insertAux 0x008ED778u /* local_static_verified */
#define SEAM_File_exists 0x0011D5A4u /* local_static_verified */

/* == native command system == */
#define SEAM_MinecraftCommands_ctor       0x00363C90u
#define SEAM_CommandParser_getCommandName 0x006BDE2Cu
#define SEAM_CommandParser_isValidSlashCommand 0x006BDFD4u
#define SEAM_CommandParser_originHasPermissions 0x006BE0F4u
#define SEAM_CommandParser_getCommandJson 0x00208C00u
#define SEAM_CommandParser_getArgumentStartPosition 0x006BE304u
#define SEAM_CommandParser_scopeOverloads 0x006BDA54u
#define SEAM_CommandParser_getCommand     0x006BCF50u
#define SEAM_CommandParser_extractInputs  0x006BD0B4u
#define SEAM_Command_getCommandOverload   0x00629D50u
#define SEAM_CommandPayload_validate       0x003EE200u
#define SEAM_CommandOverload_getCallback   0x006D22ACu
#define SEAM_MinecraftCommands_validatePermissions 0x006ECADCu
#define SEAM_MinecraftCommands_scheduleExecution   0x0038EDB8u
#define SEAM_MinecraftCommands_executeCommand       0x00362FBCu
#define SEAM_CommandPayloadParser_parseParameters 0x003ED7BCu
#define SEAM_CommandParser_registerStringEnum 0x00208798u
#define SEAM_CommandParser_commandMapOffset 0x1Cu
#define SEAM_CommandParser_commandHeaderOffset 0x2Cu
#define SEAM_CommandParser_commandCountOffset 0x30u
#define SEAM_CommandParser_commandNameMapAllocatorOffset 0x38u
#define SEAM_CommandParser_commandNameMapHeaderOffset 0x48u
#define SEAM_CommandParser_commandNameMapCountOffset 0x4Cu
#define SEAM_CommandMap_cloneSubtree      0x0087104Cu
#define SEAM_CommandNameMap_cloneSubtree  0x0086806Cu
#define SEAM_CommandPropertyBag_jsonOffset 0x08u      /* PropertyBag inline Json::Value */
#define SEAM_CommandTarget_serializeToken  0x0020A248u
#define SEAM_CommandTarget_fromPropertyBag 0x0020B2D8u
#define SEAM_CommandTarget_parseProperty   0x0020BCFCu
#define SEAM_CommandTarget_getMatchCount   0x00209F18u
#define SEAM_CommandTarget_begin           0x0020BCD0u
#define SEAM_CommandTarget_end             0x0020BCC4u
#define SEAM_CommandTarget_Iterator_dereference 0x0020CB68u
#define SEAM_CommandTarget_Iterator_increment 0x0020CBB0u
#define SEAM_CommandTarget_Iterator_notEqual 0x006BE458u
#define SEAM_CommandTarget_resolve         0x0020C118u
#define SEAM_CommandTargetTypeMap_cloneSubtree 0x0087ACF4u
#define SEAM_CommandTestResults_vectorDtor  0x008FC0A0u
#define SEAM_IntellisenseOverload_vectorDtor 0x008FC17Cu
#define SEAM_Json_Value_lookupGstd         0x0071D368u
#define SEAM_Json_Value_asInt              0x0071CB5Cu
#define SEAM_Json_Value_asInt64            0x0071C33Cu
#define SEAM_Json_Value_asBool             0x0071CC40u
#define SEAM_Json_Value_asDouble           0x0071CE04u
#define SEAM_Json_Value_isDouble           0x0071D054u
#define SEAM_Json_Value_isNull             0x0071CDDCu
#define SEAM_Json_Value_isInt              0x0071CC30u
#define SEAM_Json_Value_isUInt             0x0071CDF0u
#define SEAM_Json_Value_isString           0x0071D0C4u
#define SEAM_Json_Value_isObject           0x0071CBFCu
#define SEAM_Json_Value_isArrayOrNull      0x0071CF0Cu
#define SEAM_Json_Value_size               0x0071CA7Cu
#define SEAM_Json_Value_indexByUint        0x0071D4A0u
#define SEAM_Tag_constructByType           0x0057C000u /* Json::Value null constructor */
#define SEAM_Tag_copyByType                0x0057C250u /* local_static_verified */
#define SEAM_Json_Value_objectMemberCString 0x0057BA8Cu /* Json::Value::operator[](char const*) */
#define SEAM_Json_Value_append             0x0057BCDCu /* Json::Value::append */
#define SEAM_Json_Value_stdStringCtor      0x0057C49Cu /* local_static_verified */
#define SEAM_Json_Value_boolCtor           0x0057C504u /* local_static_verified */
#define SEAM_Json_Value_doubleCtor         0x0057C51Cu /* local_static_verified */
#define SEAM_Json_Value_intCtor            0x0057C53Cu /* local_static_verified */
#define SEAM_Json_Value_int64Ctor          0x0057C570u /* local_static_verified */
#define SEAM_Tag_destructByType             0x0057C5A0u /* local_static_verified */
#define SEAM_Json_Value_assign              0x0057C6D4u /* Json::Value assignment */
#define SEAM_Json_Value_getOrCreate         0x0057C754u /* Json::Value object member */
#define SEAM_CommandResult_success           0x00A35A0Au
#define SEAM_CommandResult_notFound          0x00A359BAu
#define SEAM_CommandResult_permission        0x00A359BEu
#define SEAM_CommandResult_syntax            0x00A359C6u
#define SEAM_CommandResult_communications    0x00A359D6u
#define SEAM_CommandResult_chatMuted         0x00A359DEu
#define SEAM_CommandResult_global            SEAM_CommandResult_notFound
#define SEAM_TagResourceReader_constructor  0x0057F0E0u
#define SEAM_TagResourceReader_destructor   0x0057F1A0u
#define SEAM_TagResourceReader_parse        0x0057E080u

/* == MCPE login/authentication == */
#define SEAM_LoginPacket_write          0x006A6A38u
#define SEAM_BinaryStream_writeString   0x001BC29Cu
#define SEAM_BinaryStream_writeByte     0x001BC7DCu
#define SEAM_BinaryStream_writeSignedBigEndianInt 0x001BC70Cu
#define SEAM_BinaryStream_writeBool     0x001BC824u
#define SEAM_WebToken_create            0x006655E0u

/* == NBT / tags == */
#define SEAM_CompoundTag_ctor         0x00182464u
#define SEAM_CompoundTag_dtor         0x00182644u
#define SEAM_CompoundTag_putShort     0x001820B4u
#define SEAM_CompoundTag_getShort     0x006A11E0u
#define SEAM_CompoundTag_getList      0x006A0C30u
#define SEAM_CompoundTag_putList      0x00181A30u
#define SEAM_ListTag_constructor      0x0062F070u
#define SEAM_ListTag_appendTag        0x0062EDE8u
#define SEAM_ListTag_size             0x0072BC20u
#define SEAM_ListTag_getCompound      0x0072BAC8u

/* == level / dimension == */
#define SEAM_Level_getTime            0x0073F3B8u
#define SEAM_Level_setTime            0x005CE60Cu
#define SEAM_Level_destroyBlock        0x005C5CBCu
#define SEAM_Level_getDifficulty      0x0073F30Cu
#define SEAM_Level_setDifficultyVtableOffset 0x54 /* verified packet handlers dispatch here */
#define SEAM_MinecraftGame_getOptions 0x00225EC4u /* local_static_verified */
#define SEAM_Options_getFloat         0x00630718u /* local_static_verified */
#define SEAM_Options_setInt           0x00632AECu /* local_static_verified */
#define SEAM_Options_difficulty       0x00A31C8Cu /* difficulty descriptor used by world settings */
#define SEAM_Options_difficultyValueOffset 0x9C    /* Options integer selected by that descriptor */
#define SEAM_Level_getGameRules       0x0067CB58u  /* returns Level+0x90 */
#define SEAM_Level_getDimension       0x00720854u
#define SEAM_Level_getLevelStorage    0x005C85D8u
#define SEAM_Level_forEachPlayer      0x005C6B6Cu
#define SEAM_Level_packetSenderOffset 0x244u
#define SEAM_Level_isClientSide       0x00720908u
#define SEAM_Level_broadcastLevelEvent 0x005C9F48u /* (level, eventId, ctx, param, flags) */
#define SEAM_Dimension_chunkSourceOffset 0xCC      /* Dimension+0xCC = ChunkSource* */
#define SEAM_Dimension_weatherOffset    0xD0      /* Dimension+0xD0 = Weather* */
#define SEAM_Weather_constructor      0x00649698u
#define SEAM_Weather_tick             0x006489B8u
#define SEAM_Level_liveRainLevel      0x130       /* Level+0x130 f32 */
#define SEAM_Level_liveRainTime       0x134       /* Level+0x134 i32 */
#define SEAM_Level_liveLightningLevel  0x138      /* Level+0x138 f32 */
#define SEAM_Level_liveLightningTime   0x13C      /* Level+0x13C i32 */

/* == gamerules == */
#define SEAM_GameRules_getBool        0x0073D1F8u
#define SEAM_GameRule_getBool         0x00732FA4u
#define SEAM_GameRule_setBool         0x006511E8u
#define SEAM_GameRule_setInt          0x00651184u
#define SEAM_GameRule_setFloat        0x0065124Cu
#define SEAM_GameRules_map_offset     0x90        /* Level+0x90 = inline GameRules */
#define SEAM_Gamerule_header_offset   0x10        /* rules+0x10 = rb-tree header ptr */
#define SEAM_Gamerule_type_offset     0x02        /* rule+0x02 = type byte */
#define SEAM_Gamerule_value_offset    0x04        /* rule+0x04 = value */

/* == block access == */
#define SEAM_BlockSource_setBlockAndDataIdData 0x00176AC0u
#define SEAM_BlockSource_getBlockId    0x00174648u
#define SEAM_BlockSource_getBlockIdAndData 0x00176778u
#define SEAM_BlockSource_getBlockEntity 0x001762A8u /* (BlockSource, BlockPos) */
#define SEAM_BlockSource_getAboveTopSolidBlock 0x00178C88u
#define SEAM_BlockSource_getDimensionId 0x0069DF64u
#define SEAM_BlockSource_getDimension  0x0069DE2Cu
#define SEAM_BlockSource_getLevel      0x0069DF9Cu
#define SEAM_BlockSource_getLevelChunkFromBlockPos 0x00174964u
#define SEAM_BlockSource_isEmptyBlock  0x0017AF9Cu
#define SEAM_BlockSource_updateNeighborsAt 0x00177C24u
#define SEAM_BlockSource_getTopRainBlockPos 0x00177EF0u
#define SEAM_BlockSource_getMaterialXYZ 0x001751A4u
#define SEAM_Block_hasProperty         0x0071DF24u
#define SEAM_Block_solidById           0x00B10020u
#define SEAM_SkullBlockEntity_setSkullType 0x00343BACu
#define SEAM_SkullBlockEntity_rotationOffset 0x64u
#define SEAM_Material_isType           0x00733778u
#define SEAM_BlockEntity_isType        0x00173EC8u /* entity type field +0x4C */
#define SEAM_BlockEntity_setChanged    0x00172FB0u
#define SEAM_ChunkSource_getFeatureId  0x0017C654u
#define SEAM_ChunkSource_findNearestFeature 0x0017E898u
#define SEAM_ChunkPos_ctorBlockPos     0x0064CA6Cu
#define SEAM_GetLevelChunk             0x0017AB3Cu
#define SEAM_ChunkBlockPos_ctorBlockPos 0x00206DD8u
#define SEAM_Block_lookupByName        0x005BD34Cu
#define SEAM_BlockRegistry_byNumericId 0x00B10520u
#define SEAM_ItemRegistry_byNameHash   0x00B0D728u
#define SEAM_ItemRegistry_byNumericId  0x00B0CEF0u
#define SEAM_SubChunk_idOffset         4096       /* metadata starts after IDs */
#define SEAM_Level_difficultyOffset    0x154      /* Level+0x154 i32 */

/* == item / inventory == */
#define SEAM_ItemInstance_itemCountAuxCtor 0x001D2510u
#define SEAM_ItemInstance_idCountAuxCtor    0x001D2894u
#define SEAM_ItemInstance_dtor              0x001D295Cu
#define SEAM_ItemRenderer_getInstance       0x001D2E04u
#define SEAM_ItemRenderer_renderGuiItemNew  0x001D2F28u
#define SEAM_ItemInstance_init              0x001D1A10u
#define SEAM_ItemInstance_isNull            0x006B2954u
#define SEAM_ItemInstance_isArmor           0x006B15A0u
#define SEAM_ItemInstance_getSlotForItem    0x00666244u
#define SEAM_ItemInstance_getId             0x006B2924u
#define SEAM_ItemInstance_getMaxStackSize   0x006B1DA0u
#define SEAM_Item_allowOffhandOffset        0x2Au /* Item +0x2A; target registration stores and framework layout agree */
#define SEAM_FillingContainer_addItem       0x00317650u /* vfunc +0x68 insertion/stacking */
#define SEAM_FillingContainer_clearInventory 0x00317310u
#define SEAM_FillingContainer_subItem       0x00316FF8u
#define SEAM_PlayerInventoryProxy_getInventory 0x003170DCu
#define SEAM_PlayerInventoryProxy_getItem 0x00703998u
#define SEAM_PlayerInventoryProxy_getSelectedContainer 0x00703790u
#define SEAM_PlayerInventoryProxy_setSelectedItem 0x003F90DCu
#define SEAM_PlayerInventoryProxy_selectedSlot_offset 0x08
#define SEAM_Player_getSupplies             0x00726090u
#define SEAM_Player_setMainhandItem         0x003F90CCu
#define SEAM_Mob_setItemSlot                0x004E3D08u
#define SEAM_Mob_setArmorSlot               0x004EDA54u
#define SEAM_Mob_setOffhandItem             0x004E6054u
#define SEAM_FillingContainer_getItem       0x006E0634u
#define SEAM_FillingContainer_getNextEmptySlot 0x00317554u
#define SEAM_FillingContainer_getLinkedSlot 0x006E0030u /* (container, logical slot) */
#define SEAM_FillingContainer_getLinkedSlotsCount 0x006E03ECu
#define SEAM_FillingContainer_linkSlot      0x00318698u /* (container, logical, physical) */
#define SEAM_ItemInstance_isHorseArmorItem  0x006B1F3Cu
#define SEAM_EnchantUtils_applyEnchant      0x001C3134u
#define SEAM_BlockPointerVector_append      0x0090113Cu

/* == entity / spawner == */
#define SEAM_EntityClassTree_isInstanceOf   0x002C84D0u /* exact target EntityClassTree bitmask predicate */
#define SEAM_EntityDefinition_InitById      0x0048AF00u
#define SEAM_EntityDefinition_FromEntityId  0x0014C484u
#define SEAM_EntityDefinition_BuildSpawnData 0x0048B3A4u
#define SEAM_Spawn_MobFromDefinition        0x006450A0u
#define SEAM_EntityFactory_Create           0x00213254u
#define SEAM_EntityFactory_createEntity     0x00212968u
#define SEAM_Level_addEntity                0x005CE734u
#define SEAM_EntityFactory_descriptors      0x0098809Cu
#define SEAM_Enchant_registry               0x00B0C370u
#define SEAM_MobEffect_nameMap              0x00B095A0u
#define SEAM_MobEffectNameMap_lowerBound    0x008DE750u
#define SEAM_EntityTypeFromString           0x0013E4D4u
#define SEAM_EntityType_toString             0x0013A4A8u
#define SEAM_EntityFactory_CapacityBypass   0x00A33894u
#define SEAM_Entity_getLevel                0x005F741Cu
#define SEAM_Entity_hurt                    0x005F3368u
#define SEAM_Entity_isAlive                 0x00724EC8u
#define SEAM_Entity_hasCategory              0x00723678u
#define SEAM_Entity_synchedDataOffset        0x230u
#define SEAM_DamageSource_ctor              0x00395B8Cu
#define SEAM_EntityDamageSource_ctor        0x00487AA4u
#define SEAM_Entity_getUniqueID             0x00723618u
#define SEAM_Level_fetchEntity               0x00720768u
#define SEAM_Entity_drop                    0x005F2F38u
#define SEAM_Entity_uniqueIdHash            0x006EFBD8u
#define SEAM_EntityRegistry_fetch           0x004CF7B4u
#define SEAM_EntityRegistry_add             0x004CFBDCu
#define SEAM_EntityRegistry_insert2         0x004D0350u
#define SEAM_Spawn_defaultDefGroup          0x0098A38Cu /* data */
#define SEAM_MinecraftGame_worldsVector     0x30        /* game+0x30 begin/end/cap */
#define SEAM_MinecraftGame_leaveGame        0x00231454u /* local_static_verified */
#define SEAM_World_entityRegistryOffset     0x30        /* world+0x30 */
#define SEAM_Entity_posOffset               0x1C0       /* Entity+0x1C0 Vec3; verified by Level::addEntity */
#define SEAM_Entity_regionOffset            0x210
#define SEAM_Entity_teleportTo               0x005E9208u /* mapped Entity teleport pipeline */
#define SEAM_Entity_teleportVtableOffset     0x60u
#define SEAM_Entity_setRot                   0x005F6A88u /* (Entity, Vec2 pitch/yaw) */
#define SEAM_Entity_getDimensionId            0x00724724u
#define SEAM_Entity_xRotOffset                0x1F0u
#define SEAM_Entity_yRotOffset                0x1F4u
#define SEAM_Entity_packetSenderOffset      0x18EC      /* Entity+0x18EC sender ptr */
#define SEAM_Entity_containerComponentOffset 0xD6C      /* Entity+0xD6C = optional ContainerComponent* */
#define SEAM_ContainerComponent_containerOffset 0x04    /* component +0x04 = FillingContainer* */
#define SEAM_ContainerComponent_ownerOffset 0x08        /* component +0x08 = owning Entity* */
#define SEAM_Entity_damageVtableSlot        0x29C       /* vtable slot index for damage */
#define SEAM_Mob_effectVectorBegin          0x1014      /* Mob+0x1014 void** */
#define SEAM_Mob_effectVectorEnd            0x1018      /* Mob+0x1018 void** */
#define SEAM_MobEffectInstance_empty        0x00B09650u
#define SEAM_Mob_onEffectRemovedVtableOffset 0x474u
#define SEAM_MobEffectInstance_ctor         0x0036B9F4u
#define SEAM_MobEffectInstance_operatorNotEqual 0x006EDDECu
#define SEAM_Mob_addEffect                  0x004EDD7Cu
#define SEAM_Entity_distanceToVec3          0x00723460u
#define SEAM_Mob_sendInventory              0x00713B00u
#define SEAM_Boat_control                    0x0052F87Cu
#define SEAM_Boat_setPaddleState             0x0052EC30u
#define SEAM_Boat_riderVectorOffset          0x324u
#define SEAM_Boat_leftPaddleForceOffset      0xEA4u
#define SEAM_Boat_rightPaddleForceOffset     0xEB8u
#define SEAM_FishingHook_hitCheck            0x0018D58Cu
#define SEAM_SynchedEntityData_getDouble      0x006F2F78u

/* == player == */
#define SEAM_Player_getPlayer               0x0025118Cu /* from MinecraftGame* */
#define SEAM_Player_enderChestContainerOffset 0x1910    /* serialized by Player add/readAdditionalSaveData */
#define SEAM_Player_setPlayerGameType       0x00601900u
#define SEAM_Player_setRespawnPosition      0x00602328u /* reference-correlated static-verified */
#define SEAM_GameMode_toString              0x001C99C0u
#define SEAM_Player_teleportTo              0x005FF364u
#define SEAM_Player_setGameType             0x00601900u
#define SEAM_Player_setPermissionsLevel     0x00665F38u
#define SEAM_Player_getPermissionsLevel     0x00735690u
#define SEAM_Player_addLevels               0x00607380u
#define SEAM_Player_addExperience           0x005FFB80u
#define SEAM_Player_getMainhandItem         0x003F9044u
#define SEAM_Player_damageVtableSlot        0x29C
#define SEAM_ClientCommandOrigin_ctor       0x003D6330u
#define SEAM_CommandOrigin_fromCommandOriginData 0x00207CE4u
#define SEAM_CommandOriginData_ctor         0x00354F3Cu
#define SEAM_CommandOriginData_copyCtor     0x00354CD4u
#define SEAM_CommandOriginData_dtor         0x00354F58u
#define SEAM_CommandOrigin_baseCtor         0x0020821Cu
#define SEAM_VirtualCommandOrigin_vtable    0x009BFB34u
#define SEAM_EntityCommandOrigin_vtable     0x009BC1C8u
#define SEAM_CommandParser_getEnchantmentTypes 0x008CB9E4u
#define SEAM_CommandParser_getEffectTypes   0x008CBE70u
#define SEAM_CommandParser_getItemTypeNames 0x008CBFACu
#define SEAM_CommandParser_getBlockTypeNames 0x008CB16Cu
#define SEAM_CommandParser_getFeatureTypes  0x008CBD80u
#define SEAM_CommandParser_getBlockSlotTypes 0x008CB3E0u
#define SEAM_CommandParser_getEntitySlotTypes 0x008CB44Cu
#define SEAM_RequestCommandExecution        0x006ED3D8u
#define SEAM_HasCommandsEnabled             0x0073F32Cu
#define SEAM_SetCommandsEnabled             0x001A8E14u
#define SEAM_CopyEntityName                 0x00724730u
#define SEAM_ClientInstance_getLevel        0x00124DC8u
#define SEAM_ClientInstance_getGui          0x00225EACu
#define SEAM_Level_levelDataOffset          0x88
#define SEAM_LevelData_achievementsDisabledOffset 0xE6

/* == network / packet == */
#define SEAM_game_protocol_version          0x0091C4C0u /* writable USA protocol global; startExternalNetworkWorld compares row+0x18 against it */
#define SEAM_nn_svc_ControlMemory           0x001065ECu /* library_source_verified */
#define SEAM_nn_svc_CreateMemoryBlock       0x00118E40u /* library_source_verified */
#define SEAM_nn_srv_GetServiceHandle        0x004C5B88u /* library_bsim_single_variant_verified */
#define SEAM_TextPacket_ctor                0x0016B52Cu
#define SEAM_TextPacket_dtor                0x0016B694u
#define SEAM_LoopbackSend                   0x003F4908u
#define SEAM_LoopbackSendBroadcast          0x003F44B8u
#define SEAM_LoopbackPacketSender_sendVtableOffset 0x08u
#define SEAM_MinecraftPackets_createPacket  0x00328798u
#define SEAM_LoopbackPacketSender_sendTo     0x003F46C4u
#define SEAM_NetworkHandler_send             0x00273758u
#define SEAM_NetworkHandler_Connection_constructor 0x0027275Cu
#define SEAM_NetworkHandler_getEncryptedPeerForUser 0x002733A8u
#define SEAM_NetworkHandler_onHandshake      0x00273698u
#define SEAM_ClientToServerHandshakePacket_vtable 0x009C5B4Cu
#define SEAM_ClientToServerHandshakePacket_dtor 0x004A7780u
#define SEAM_ResourcePackClientResponsePacket_vtable 0x009C60C8u
#define SEAM_ResourcePackClientResponsePacket_dtor 0x004D15CCu
#define SEAM_ClientNetworkHandler_handleServerToClientHandshakePacket 0x003ED0CCu
#define SEAM_ClientNetworkHandler_handleResourcePacksInfoPacket 0x0022D0F8u
#define SEAM_ClientNetworkHandler_handleResourcePacksStackPacket 0x003EC9A8u
#define SEAM_ClientNetworkHandler_handleStartGamePacket 0x0048D284u
#define SEAM_StartGamePacket_read           0x002DE410u
#define SEAM_CraftingDataPacket_read        0x003906E4u
#define SEAM_ClientNetworkHandler_allowIncomingPacketId 0x003EA238u
#define SEAM_RequestChunkRadiusPacket_vtable 0x009C4498u
#define SEAM_RequestChunkRadiusPacket_dtor 0x0047A114u
#define SEAM_LocalPlayer_requestChunkRadius 0x001971D8u
#define SEAM_BatchedNetworkPeer_setBatching  0x0038A8F4u
#define SEAM_CompressedNetworkPeer_setCompression 0x0040A434u
#define SEAM_ReadOnlyBinaryStream_getVarInt64 0x003F9A58u
#define SEAM_ReadOnlyBinaryStream_readVarInt 0x003FA0CCu
#define SEAM_ReadOnlyBinaryStream_getBool    0x003F9D90u
#define SEAM_ReadOnlyBinaryStream_getFloat   0x003F9DD8u
#define SEAM_ReadOnlyBinaryStream_getUnsignedInt64 0x003F9B5Cu
#define SEAM_ReadOnlyBinaryStream_getUnsignedVarInt 0x003F9BA4u
#define SEAM_ReadOnlyBinaryStream_getString 0x003F9E00u
#define SEAM_GameRules_readVector            0x007E2878u
#define SEAM_GameRules_setRulesFromVector    0x00672B5Cu
#define SEAM_GstdString_dtor                 0x002FEBBDu
#define SEAM_GstdString_assign               0x002FFFD0u
#define SEAM_RakNet_SystemAddress_constructorFromString 0x00609D60u
#define SEAM_RakNet_SystemAddress_fromString 0x00609B0Cu
#define SEAM_RakNetSocket2_3DS_initialize 0x0061CA14u
#define SEAM_RakNetSocket2_3DS_send 0x0061CA8Cu
#define SEAM_RakNetSocket2_3DS_receiveFrom 0x00247540u
#define SEAM_RakNetSocket2_3DS_destructorDelete 0x0061CAB8u
#define SEAM_RakNetSocket2_3DS_destructor 0x0061CB0Cu
#define SEAM_NetworkHandler_host 0x00273730u
#define SEAM_NetworkHandler_connect 0x002738F8u
#define SEAM_NetworkHandler_onNewOutgoingConnection 0x00273544u /* local_static_verified */
#define SEAM_ClientNetworkHandler_onConnect 0x00490C70u /* local_static_verified */
#define SEAM_ClientNetworkHandler_onUnableToConnect 0x0048C540u /* local_static_verified */
#define SEAM_MinecraftGame_leaveGame 0x00231454u /* local_static_verified */
#define SEAM_NetworkHandler_disconnect 0x0026CD28u /* local_static_verified */
#define SEAM_RakNetInstance_disconnect 0x00286F1Cu
#define SEAM_SelectServerScreen_setupPositions 0x00268BE0u
#define SEAM_SelectServerScreen_setupScreen 0x00269F44u
#define SEAM_SelectServerScreen_render 0x0026A160u
#define SEAM_SelectServerScreen_destructor 0x0026A320u
#define SEAM_SelectServerScreen_appendLabels 0x00269B64u
#define SEAM_SelectServerScreen_rebuild 0x002694F8u
#define SEAM_LocalWirelessNetwork_getOrCreate 0x00107A40u
#define SEAM_LocalWirelessNetwork_getAvailabilityFlags 0x00245054u
#define SEAM_LocalWirelessNetwork_start 0x002470F0u
#define SEAM_LocalWirelessNetwork_finalizeIfStateOne 0x00246F9Cu
#define SEAM_LocalWirelessNetwork_pollConnectionStatus 0x0024719Cu
#define SEAM_ScreenChooser_setDisconnectScreen 0x0023D4C0u
#define SEAM_MinecraftGame_checkWirelessError 0x004C9470u
#define SEAM_MinecraftGame_checkWirelessError_call 0x002316E0u
#define SEAM_GameSession_destroyClient 0x00681658u
#define SEAM_NetworkIdentifier_copyFromEntity 0x00725F7Cu
#define SEAM_ItemInstance_copyAssign         0x001D2C3Cu
#define SEAM_InventoryActionPacket_dtor      0x0041E4C0u
#define SEAM_MobEquipmentPacket_dtor         0x003A1FF8u
#define SEAM_PlaySoundPacket_dtor           0x002D73CCu
#define SEAM_SetTimePacket_vtable           0x009A7728u
#define SEAM_StopSoundPacket_vtable         0x009B1A00u
#define SEAM_StopSoundPacket_dtor           0x002DF3E4u
#define SEAM_StopSoundPacket_write          0x001BC7B4u
#define SEAM_StopSoundPacket_read           0x002DF3BCu
#define SEAM_SetTitlePacket_dtor            0x0028C884u

/* == UI / rendering == */
#define SEAM_TextWidth                      0x0071AD28u
#define SEAM_DrawText                       0x0055BB84u
#define SEAM_FillRect                       0x006CFF68u
#define SEAM_GuiComponent_setShaderColor    0x001A9948u
#define SEAM_Tessellator_beginMaxVertices   0x001B1BF0u
#define SEAM_Tessellator_vertexUVFloats     0x001B300Cu
#define SEAM_Tessellator_drawMaterialTexture 0x001B1984u
#define SEAM_Tessellator_end                0x001B15BCu
#define SEAM_Mesh_renderTextured            0x00717658u
#define SEAM_Mesh_dtor                      0x00509214u
#define SEAM_Sprite_ctor                    0x0055FE08u
#define SEAM_Sprite_render                  0x0055FDC4u
#define SEAM_TexturePtr_deref               0x004F0854u
#define SEAM_TextureLazyLoad                0x0065FAE4u
#define SEAM_MaterialPtr_dtor               0x004F153Cu
#define SEAM_NinePatchLayer_setSize         0x0027F0ECu
#define SEAM_UiTessellator                  0x00AC42D8u
#define SEAM_UiNinePatchMaterial            0x00ABFC80u
#define SEAM_UiShaderColorGlobal            0x00B2DD40u
#define SEAM_UiWhiteColor                   0x00B2E4B0u
#define SEAM_UiBlackColor                   0x00B2E4E0u
#define SEAM_RenderDeviceBase_instance      0x00A39A0Cu
#define SEAM_GuiData_uiTexturedMaterialOffset 0x2C8u
#define SEAM_GuiData_iconsTextureOffset     0x2D4u
#define SEAM_MinecraftGame_fontOffset       0x5C
#define SEAM_MinecraftGame_textureManagerOffset 0x58
#define SEAM_MinecraftGame_screenChooserOffset 0x148
#define SEAM_ResourceLocation_ctor          0x0033C630u
#define SEAM_ResourceLoader_loadTexture     0x006E5F38u
#define SEAM_Texture3DS_load3DST            0x00249838u
#define SEAM_AppPlatform_singleton          0x0010FFD4u
#define SEAM_GuiButton_ctor                 0x005DB924u
#define SEAM_GuiButton_buildBackground      0x005DB4B0u
#define SEAM_BaseButton_isPressed           0x005E6338u
#define SEAM_RenderStateLogicOp_isDstColorUsed 0x0071A9ECu
#define SEAM_ControlAlloc                   0x00110E50u
#define SEAM_ControlLock                    0x00123168u
#define SEAM_ControlUnlock                  0x001231A8u
#define SEAM_ControlReference               0x00119C64u
#define SEAM_CommandShared_assign            0x008B1150u
#define SEAM_SharedPush                     0x008F9788u
#define SEAM_SharedRelease                  0x008B1074u
#define SEAM_HudButtonFactory               0x007E4328u
#define SEAM_HudButtonSkin                  0x005D9FA8u
#define SEAM_HudButtonSkin_iconOffsetX      0xB0
#define SEAM_HudButtonSkin_iconOffsetY      0xB4
#define SEAM_MinecraftGame_playSound        0x00230B18u
#define SEAM_SoundEngine_play                0x001ADB58u
#define SEAM_MinecraftGame_soundEngineOffset 0xDC
#define SEAM_MinecraftGame_pushScreen       0x00226054u
#define SEAM_MinecraftGame_schedulePopScreen 0x001A82FCu
#define SEAM_MinecraftGame_getClientInstance 0x0022BF04u /* local_static_verified */
#define SEAM_GuiData_clearTitleMessages     0x00252084u
#define SEAM_GuiData_resetTitle             0x001F202Cu
#define SEAM_GuiData_setTitle               0x0062E0ACu
#define SEAM_GuiData_setSubtitle            0x001F2078u
#define SEAM_GuiData_setActionBarMessage    0x0062DC0Cu
#define SEAM_GuiData_setTitleAnimationTimes 0x001F20C4u
#define SEAM_Screen_ctor                    0x00621078u
#define SEAM_Screen_render                  0x00620E94u
#define SEAM_Screen_updateComponentSelections 0x00620A84u
#define SEAM_Screen_handleMappedButton      0x0061FFD4u
#define SEAM_ClientInstance_getClientInputHandler 0x0012AAD4u
#define SEAM_ClientInputHandler_lookupBinding 0x0012D628u
#define SEAM_MinecraftInputHandler_handleChatAction 0x003E8904u
#define SEAM_InputPlatform_pollControllers 0x00107F30u /* local_static_verified stock HID poll owner */
#define SEAM_GameController_statusOffset   0x160u      /* stock ExtraPadStatus updated by the poll owner */
#define SEAM_UiButtonClickSoundHash         0x7E943735u
#define SEAM_CreateWorldLabelColorPtr       0x0040BC94u  /* data: Color* used by stock option labels */
#define SEAM_GuiComponent_drawString        0x0028B3FCu  /* shared integer-position UI text path */
#define SEAM_GuiComponent_drawCenteredString 0x0028B970u /* local_static_verified */
#define SEAM_GuiComponent_drawCenteredAtString 0x0028B8F0u /* local_static_verified */

/* == screen hooks == */
#define SEAM_InGamePlayScreen_setup         0x006787B8u
#define SEAM_InGamePlayScreen_buttonPressed 0x00677044u
#define SEAM_InGamePlayScreen_render        0x00678FD0u
#define SEAM_InGamePlayScreen_renderTopHud  0x00676F34u
#define SEAM_InGamePlayScreen_allowsHotbarInput 0x0073D354u
#define SEAM_ClientNetworkHandler_handleTextPacket 0x003EA258u
#define SEAM_GuiData_displayClientMessage 0x0062CBF0u
#define SEAM_GuiData_appendMessageEntry   0x0062D788u
#define SEAM_SystemMessagesScreen_pushMessage 0x0011A680u
#define SEAM_CreateWorldScreen_setup        0x003F74F4u
#define SEAM_EditWorldScreen_setup          0x002C7ECCu
#define SEAM_CreateWorldScreen_buttonPressed 0x003F6BE4u
#define SEAM_EditWorldScreen_buttonPressed   0x002C7058u
#define SEAM_CreateWorldScreen_start        0x003F65F0u
#define SEAM_WorldSettingsScreen_render     0x0040D470u
#define SEAM_PauseScreen_render             0x001A1920u
#define SEAM_AchievementWarningScreen_ctor  0x004966A8u
#define SEAM_AchievementWarningScreen_buttonPressed 0x00495CD4u
#define SEAM_PushAchievementWarning   0x0023EF70u
#define SEAM_PushSavingScreen          0x0023BA6Cu
#define SEAM_ScreenChooser_schedulePopScreen 0x0023C55Cu
#define SEAM_ScreenChooser_pushProgressScreen 0x0023CC30u /* local_static_verified */
#define SEAM_WorldSelection_openExisting     0x0023AEBCu /* local_static_verified */
#define SEAM_FrameworkCallbackTable_getEntry 0x00106670u /* local_static_verified */
#define SEAM_BackGroundWorker_sync           0x00107D6Cu /* local_static_verified */
#define SEAM_MinecraftGame_loadClientResources 0x0022874Cu
#define SEAM_AchievementsScreen_setup       0x00389494u
#define SEAM_AchievementsScreen_render      0x0038954Cu
#define SEAM_ContainerScreen_renderLabels   0x003B0B60u  /* shared container title-label pass */

/* == options screen == */
#define SEAM_OptionsScreen_createCategorySprites 0x002353A0u
#define SEAM_OptionsScreen_render        0x00236BECu
#define SEAM_OptionsScreen_buttonPressed 0x00236240u
#define SEAM_OptionsScreen_ownerButtonPressed 0x00749334u /* OptionsScreen vtable owner callback at +0x1F0 */
#define SEAM_OptionsScreen_buildSlider      0x00234D40u
#define SEAM_Slider_updateFromTouch         0x00621A94u
#define SEAM_Slider_usesDirectionalInput    0x006C1154u
#define SEAM_TouchPad_getX                  0x0019C02Cu
#define SEAM_TouchPad_getY                  0x0019C03Cu
#define SEAM_MinecraftGame_getScreen        0x0012D544u
#define SEAM_OptionsScreen_buildSwitch      0x00234F10u
#define SEAM_OptionsRow_ctor           0x0044E55Cu
#define SEAM_OptionItem_layoutChildren 0x001A046Cu
#define SEAM_OptionItem_render         0x001A08D4u
#define SEAM_OptionItem_getLocalizedName 0x006A8108u
#define SEAM_SwitchButton_buildThumb    0x001DD020u
#define SEAM_Options_brightness             0x00A31D54u
#define SEAM_Options_fov                    0x00A31D9Cu
#define SEAM_Options_inGameOffset      0x118
#define SEAM_Options_gameVectorOffset  0x14C
#define SEAM_Options_graphicsVectorOffset   0x164
#define SEAM_Options_viewBobbing             0x00A31C7Cu
#define SEAM_Options_fancyGraphics           0x00A31C94u
#define SEAM_Options_fancySkies              0x00A31D2Cu
#define SEAM_Options_hideGui                 0x00A31CCCu
#define SEAM_Shared_copyFromParts           0x008AF99Cu
#define SEAM_OptionsVector_push             0x008F8DB4u
#define SEAM_Shared_localDtor               0x008AFA58u
#define SEAM_OptionBuildResult_dtor         0x008B0154u

/* == first-person hand visibility == */
#define SEAM_ItemInHandRenderer_render      0x0039A87Cu

/* == world-entry ProgressScreen == */
#define SEAM_ProgressScreen_setup           0x002852E4u
#define SEAM_ProgressScreen_render          0x002856F8u
#define SEAM_ProgressScreen_dtor            0x00285C2Cu
#define SEAM_ProgressScreen_clientReady     0x006C16A8u /* exact internal predicate used by stock ProgressScreen */
#define SEAM_ProgressScreenTask_constructor 0x004CD178u /* local_static_verified */
#define SEAM_ProgressScreenTask_vtable      0x009C5DD4u /* local_static_verified */
#define SEAM_SavingScreen_render            0x001E4204u /* local_static_verified */
#define SEAM_SavingScreen_dtor              0x001E4538u /* local_static_verified */
#define SEAM_SavingScreen_exitAfterSaveOffset 0xA8u /* local_static_verified */
#define SEAM_CubemapBackgroundScreen_render 0x00462D3Cu /* local_static_verified: render(ScreenContext&) */
#define SEAM_RakNet_GetTimeMS               0x0061DA30u

/* == world settings controls == */
#define SEAM_WorldSwitchButton_ctor         0x001DD930u
#define SEAM_TexturePathStateInit           0x004F06F0u
#define SEAM_TexturePtrCopyCtor             0x004F068Cu
#define SEAM_TexturePtrCtor                 0x004F0750u
#define SEAM_TexturePtrAssign               0x004F079Cu
#define SEAM_TexturePtrDtor                 0x004F0780u
#define SEAM_ContainerAddChild              0x003BF7E4u
#define SEAM_ContainerAddChildShared        0x003BFC94u
#define SEAM_PushRow                        0x00908498u
#define SEAM_GuiText_ctor                   0x005C3000u
#define SEAM_WorldSwitch_trackResource      0x00ABFD74u  /* data: stock switch track ResourceLocation */
#define SEAM_WorldSettingsScreen_rebuildOptionGrid 0x0040B0F0u
#define SEAM_ScrollingPane_adjustContentSize 0x00240924u
#define SEAM_ScrollingPane_updateVerticalScrollIndicator 0x00240DE4u
#define SEAM_ScrollingPane_setScrollT       0x0023FAF0u
#define SEAM_ScrollingPane_snapContentOffsetToBounds 0x002409CCu
#define SEAM_ScrollingPane_advanceAnimation 0x0024148Cu
#define SEAM_ScrollingPane_handleUserInput  0x0024025Cu
#define SEAM_ScrollingPane_handleMouseInput 0x00240710u
#define SEAM_ScrollingPane_renderBatch      0x0023FBA8u
#define SEAM_ScrollingPane_vtable           0x009A7638u
#define SEAM_WorldSettingsScreen_refreshGridSelection 0x0042B018u
#define SEAM_WorldSettingsScreen_getGameModeVslot 118
#define SEAM_LevelData_ctor                 0x0067D758u /* verified stock LevelData constructor */
#define SEAM_LevelData_dtor                 0x0067D90Cu /* stock edit-world temporary cleanup */
#define SEAM_LevelData_setSpawn              0x0067CFB8u
#define SEAM_LevelData_disableAchievements  0x0067CBC4u
#define SEAM_LevelData_achievementsWillBeDisabledOnLoad 0x0073F374u
#define SEAM_SetSpawnPositionPacket_vtable   0x009C2C70u
#define SEAM_Dimension_broadcastPacketVtableOffset 0xCCu
#define SEAM_LevelData_size                 0x128u      /* exact stack object size in EditWorldScreen */
#define SEAM_LevelData_commandsEnabledOffset 0xEFu
#define SEAM_EditWorldScreen_worldNameOffset 0x120u
#define SEAM_EditWorldScreen_displayNameOffset 0x13Eu /* verified display-name field copied during EditWorld setup */
#define SEAM_ClientInstance_storageOwnerOffset 0x1Cu
#define SEAM_StorageOwner_levelStorageOffset 0x30u
#define SEAM_StorageOwner_commandsOffset 0x58u
#define SEAM_TerrainParticle_init           0x002E323Cu

/* == Skin & Model Subsystem == */
#define SEAM_SkinRepository_ctor            0x0028EDACu
#define SEAM_SkinRepository_getCurrentSkin  0x0028ED28u
#define SEAM_SkinRepository_loadJsonResource 0x0037D04Cu
#define SEAM_SkinRepository_getSkinId       0x006D0240u
#define SEAM_SkinRepository_getSkin         0x006D0458u
#define SEAM_SkinRepository_getSkinPacksByType 0x006D03F8u
#define SEAM_SelectSkinScreen_addSkinPackRow 0x0033F590u
#define SEAM_SkinPack_ctor                  0x00660334u
#define SEAM_Skin_ctor                      0x0058033Cu
#define SEAM_Skin_setSkinPack               0x00580044u
#define SEAM_SkinRepositoryEntryVector_pushBackMove 0x0090532Cu
#define SEAM_SkinPickerScreen_renderButtonTips 0x0033F0ECu
#define SEAM_SelectSkinScreen_buttonPressed 0x00340924u
#define SEAM_Screen_handleButtonDown        0x0061FC74u
#define SEAM_Screen_renderButtonTip         0x00471074u
#define SEAM_LoginPacket_parseSkinMetadata  0x003553A8u
#define SEAM_MinecraftGame_getSkinRepository 0x006C1400u
#define SEAM_MinecraftGame_getSkinGeometryGroup 0x006C14CCu
#define SEAM_ClientNetworkHandler_handleFullChunkDataPacket 0x0048F750u
#define SEAM_LevelChunk_tryChangeState              0x001574F4u
#define SEAM_LevelChunk_setFinalized                0x00156E5Cu
#define SEAM_LevelChunk_deserializeSubChunkLighting 0x0015ADD0u
#define SEAM_LevelChunk_deserializeSubChunk         0x006619A4u
#define SEAM_SubChunk_deserialize                   0x006619E0u
#define SEAM_LevelChunk_newSubChunk                 0x0015750Cu
#define SEAM_SubChunkBrightnessStorage_allocChecked 0x00484F80u
#define SEAM_SkinRepository_getSteveSkin    0x006D0370u
#define SEAM_ReadOnlyBinaryStream_getUnsignedVarInt 0x003F9BA4u
#define SEAM_ReadOnlyBinaryStream_readVarInt        0x003FA0CCu
#define SEAM_ReadOnlyBinaryStream_getBool           0x003F9D90u
#define SEAM_ReadOnlyBinaryStream_readItemInstance  0x007E10B0u
#define SEAM_ReadOnlyBinaryStream_getTypeUUID       0x007E2F50u
#define SEAM_ReadOnlyBinaryStream_readTypeShapedRecipe 0x007E1464u
#define SEAM_ReadOnlyBinaryStream_readTypeShapelessRecipe 0x007E1BB4u
#define SEAM_ReadOnlyBinaryStream_IDataInput_vtable 0x009AE7F0u
#define SEAM_ItemInstance_defaultCtor               0x001D28F4u
#define SEAM_BinaryStream_writeItemInstance         0x007D92C8u
#define SEAM_BinaryStream_writeUnsignedVarInt       0x001BC630u
#define SEAM_BinaryStream_IDataOutput_vtable        0x009B23ECu
#define SEAM_Recipes_clearRecipes                   0x00635DE0u
#define SEAM_FurnaceRecipes_clearRecipes            0x0025EBDCu
#define SEAM_Item_readUserDataDefault               0x0071B13Cu
#define SEAM_ShapedRecipe_ctorFromNetwork           0x001E967Cu
#define SEAM_ShapelessRecipe_ctorFromNetwork        0x002D9700u
#define SEAM_Player_die                             0x00605648u
#define SEAM_Mob_die                                0x004EB720u
#define SEAM_RemotePlayer_vtable                    0x009A1738u

#endif /* NUMC3DS_SEAMS_H */
