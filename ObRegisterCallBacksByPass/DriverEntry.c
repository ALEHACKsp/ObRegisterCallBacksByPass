#include <ntifs.h>


/*
һ��һ��CALLBACK_ENTRY_ITEM�Ѿ�����䣬
�������ݸ�ObpInserCallbackByAltitude������������������ 
����㲻��ϤAltitude����ֻ��һ������ֵ����ʾӦ�õ��ûص���˳�� 
�ϵ͵����ֳ�Ϊ��һ���ϸߵ����ֳ�Ϊlast�� 
������ص�ʱ���ص�������߶�ֵ���뵽�����С� 
���������ͬ�߶ȵĻص��Ѿ����б��У��򲻲����»ص�������ObpInsertCallbackByAltitude����ֵSTATUS_FLT_INSTANCE_ALTITUDE_COLLISION��ָʾ��ͻ�� 
����΢���֧�ָ߶ȴﵽ43�����ǲ����ܵ���ײ��������Ұ��Ļ��ᡣ https://msdn.microsoft.com/en-us/library/windows/hardware/ff549689%28v=vs.85%29.aspx

�ο�����:https://douggemhax.wordpress.com/2015/05/27/obregistercallbacks-and-countermeasures/#comments
*/

#define DRIVER_TAG 'xxxx'

typedef struct _OPERATION_INFO_ENTRY
{
	LIST_ENTRY    ListEntry;
	OB_OPERATION  Operation;
	ULONG         Flags;
	PVOID         Object;
	POBJECT_TYPE  ObjectType;
	ACCESS_MASK   AccessMask;
} OPERATION_INFO_ENTRY, *POPERATION_INFO_ENTRY;

LIST_ENTRY  g_OperationListHead;
FAST_MUTEX  g_OperationListLock;
PVOID       g_UpperHandle = NULL;
PVOID       g_LowerHandle = NULL;

OB_PREOP_CALLBACK_STATUS UpperPreCallback(IN PVOID RegistrationContext, IN POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	POPERATION_INFO_ENTRY NewEntry = (POPERATION_INFO_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(OPERATION_INFO_ENTRY), DRIVER_TAG);
	if (NewEntry)
	{
		NewEntry->Operation = OperationInformation->Operation;
		NewEntry->Flags = OperationInformation->Flags;
		NewEntry->Object = OperationInformation->Object;
		NewEntry->ObjectType = OperationInformation->ObjectType;
		NewEntry->AccessMask = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess; /// Same for duplicate handle
		ExAcquireFastMutex(&g_OperationListLock);
		InsertTailList(&g_OperationListHead, &NewEntry->ListEntry);
		ExReleaseFastMutex(&g_OperationListLock);
	}

	UNREFERENCED_PARAMETER(RegistrationContext);

	return OB_PREOP_SUCCESS;
}

OB_PREOP_CALLBACK_STATUS LowerPreCallback(IN PVOID RegistrationContext, IN POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	PLIST_ENTRY ListEntry;
	UNREFERENCED_PARAMETER(RegistrationContext);

	ExAcquireFastMutex(&g_OperationListLock);
	for (ListEntry = g_OperationListHead.Flink; ListEntry != &g_OperationListHead; ListEntry = ListEntry->Flink)
	{
		POPERATION_INFO_ENTRY Entry = (POPERATION_INFO_ENTRY)ListEntry;
		if (Entry->Operation == OperationInformation->Operation &&
			Entry->Flags == OperationInformation->Flags &&
			Entry->Object == OperationInformation->Object &&
			Entry->ObjectType == OperationInformation->ObjectType)
		{
			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = Entry->AccessMask;
			RemoveEntryList(&Entry->ListEntry);
			ExFreePoolWithTag(Entry, DRIVER_TAG);
			goto Release;

		}
	}
Release:
	ExReleaseFastMutex(&g_OperationListLock);

	return OB_PREOP_SUCCESS;
}

OB_OPERATION_REGISTRATION ObUpperOperationRegistration[] =
{
	{ NULL, OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE, UpperPreCallback, NULL },
	{ NULL, OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE, UpperPreCallback, NULL },
};

OB_OPERATION_REGISTRATION ObLowerOperationRegistration[] =
{
	{ NULL, OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE, LowerPreCallback, NULL },
	{ NULL, OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE, LowerPreCallback, NULL },
};

OB_CALLBACK_REGISTRATION UpperCallbackRegistration =
{
	OB_FLT_REGISTRATION_VERSION,
	2,
	RTL_CONSTANT_STRING(L"327531"),
	NULL,
	ObUpperOperationRegistration
};

OB_CALLBACK_REGISTRATION LowerCallcackRegistration =
{
	OB_FLT_REGISTRATION_VERSION,
	2,
	RTL_CONSTANT_STRING(L"327529"),
	NULL,
	ObLowerOperationRegistration
};

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	if (NULL != g_LowerHandle)
		ObUnRegisterCallbacks(g_LowerHandle);
	if (NULL != g_UpperHandle)
		ObUnRegisterCallbacks(g_UpperHandle);
	while (!IsListEmpty(&g_OperationListHead))
		ExFreePoolWithTag(RemoveHeadList(&g_OperationListHead), DRIVER_TAG);
}

UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"\\Driver\\EasyAntiCheatSys");

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS  Status;

	UNREFERENCED_PARAMETER(RegistryPath);

	InitializeListHead(&g_OperationListHead);
	ExInitializeFastMutex(&g_OperationListLock);

	ObUpperOperationRegistration[0].ObjectType = PsProcessType;
	//����ӵ�
	ObUpperOperationRegistration[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;

	ObUpperOperationRegistration[1].ObjectType = PsThreadType;
	//����ӵ�
	ObUpperOperationRegistration[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	Status = ObRegisterCallbacks(&UpperCallbackRegistration, &g_UpperHandle);
	if (!NT_SUCCESS(Status))
	{
		g_UpperHandle = NULL;
		goto Exit;
	}

	ObLowerOperationRegistration[0].ObjectType = PsProcessType;
	//����ӵ�
	ObLowerOperationRegistration[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;

	ObLowerOperationRegistration[1].ObjectType = PsThreadType;
	//����ӵ�
	ObLowerOperationRegistration[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	Status = ObRegisterCallbacks(&LowerCallcackRegistration, &g_LowerHandle);
	if (!NT_SUCCESS(Status))
	{
		g_LowerHandle = NULL;
		goto Exit;
	}

	DriverObject->DriverUnload = OnUnload;

Exit:
	if (!NT_SUCCESS(Status))
		OnUnload(DriverObject);
	return Status;
}