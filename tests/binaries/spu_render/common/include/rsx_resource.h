#ifndef COMMON_RESOURCE_H
#define COMMON_RESOURCE_H

enum REPORT_INDEX{
	REPORT_INDEX_FS_CONSTANT_PATCH0 = 0,
	REPORT_INDEX_FS_CONSTANT_PATCH1,
	REPORT_INDEX_FS_CONSTANT_PATCH2,
	REPORT_INDEX_FS_CONSTANT_PATCH3,
	REPORT_INDEX_FS_CONSTANT_PATCH4,
	REPORT_INDEX_FS_CONSTANT_PATCH5,
	REPORT_INDEX_FS_CONSTANT_PATCH6,
	REPORT_INDEX_FS_CONSTANT_PATCH7,
	REPORT_INDEX_LAST,
};

enum LABEL_INDEX{
	LABEL_INDEX_USER_BEGIN = 64,
	LABEL_INDEX_RENDERING_END = LABEL_INDEX_USER_BEGIN,
	LABEL_INDEX_COMMAND_STATUS,
	LABEL_INDEX_COMPLETE_FINISH,
	LABEL_INDEX_LAST,
};

// RSX Status
enum RSX_STATUS{
	RSX_STATUS_TRUE = CELL_OK,
	RSX_STATUS_FALSE = -1,
};
	


#endif // COMMON_RESOURCE_H