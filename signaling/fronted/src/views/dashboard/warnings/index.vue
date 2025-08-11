<template>
  <div class="alerts-panel">
    <div class="panel-header">
      <div class="title">
        <icon-exclamation-circle-fill class="icon" />
        <span>系统告警</span>
      </div>
      <a-space>
        <a-button size="mini" @click="refreshAlerts">刷新</a-button>
        <a-button size="mini" type="primary" @click="showFilter = true">筛选</a-button>
      </a-space>
    </div>
    
    <div class="alerts-content">
      <a-table
        :columns="columns"
        :data="filteredAlerts"
        :pagination="false"
        :bordered="false"
        size="small"
        row-key="id"
      >
        <template #level="{ record }">
          <a-tag :color="getLevelColor(record.level)">
            {{ getLevelText(record.level) }}
          </a-tag>
        </template>
        
        <template #status="{ record }">
          <a-tag :color="record.status === 'resolved' ? 'green' : 'red'">
            {{ record.status === 'resolved' ? '已解决' : '未处理' }}
          </a-tag>
        </template>
        
        <template #time="{ record }">
          {{ formatTime(record.time) }}
        </template>
        
        <template #operation="{ record }">
          <a-button type="text" size="mini" @click="handleAcknowledge(record)">
            <template #icon>
              <icon-check />
            </template>
            确认
          </a-button>
          <a-button type="text" size="mini" status="danger" @click="handleResolve(record)">
            <template #icon>
              <icon-close />
            </template>
            解决
          </a-button>
        </template>
      </a-table>
      
      <div v-if="filteredAlerts.length === 0" class="empty">
        <icon-exclamation />
        <p>暂无告警信息</p>
      </div>
    </div>
    
    <!-- 筛选弹窗 -->
    <a-modal
      v-model:visible="showFilter"
      title="告警筛选"
      :width="400"
      :footer="false"
    >
      <div class="filter-form">
        <a-form :model="filterForm">
          <a-form-item label="告警级别">
            <a-checkbox-group v-model="filterForm.levels">
              <a-checkbox value="critical">紧急</a-checkbox>
              <a-checkbox value="warning">警告</a-checkbox>
              <a-checkbox value="info">信息</a-checkbox>
            </a-checkbox-group>
          </a-form-item>
          
          <a-form-item label="告警状态">
            <a-checkbox-group v-model="filterForm.statuses">
              <a-checkbox value="pending">未处理</a-checkbox>
              <a-checkbox value="acknowledged">已确认</a-checkbox>
              <a-checkbox value="resolved">已解决</a-checkbox>
            </a-checkbox-group>
          </a-form-item>
          
          <a-form-item>
            <a-space>
              <a-button type="primary" @click="applyFilter">应用</a-button>
              <a-button @click="resetFilter">重置</a-button>
            </a-space>
          </a-form-item>
        </a-form>
      </div>
    </a-modal>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed } from 'vue';
import { Message } from '@arco-design/web-vue';
import dayjs from 'dayjs';

// 告警级别枚举
type AlertLevel = 'critical' | 'warning' | 'info';
type AlertStatus = 'pending' | 'acknowledged' | 'resolved';

// 告警数据结构
interface Alert {
  id: string;
  level: AlertLevel;
  title: string;
  description: string;
  source: string;
  time: Date;
  status: AlertStatus;
}

// 表格列定义
const columns = [
  {
    title: '级别',
    dataIndex: 'level',
    slotName: 'level',
    width: 80,
  },
  {
    title: '标题',
    dataIndex: 'title',
    ellipsis: true,
    tooltip: true,
  },
  {
    title: '来源',
    dataIndex: 'source',
    width: 120,
  },
  {
    title: '时间',
    dataIndex: 'time',
    slotName: 'time',
    width: 150,
  },
  {
    title: '状态',
    dataIndex: 'status',
    slotName: 'status',
    width: 80,
  },
  {
    title: '操作',
    slotName: 'operation',
    width: 120,
  },
];

// 模拟告警数据
const alerts = ref<Alert[]>([
  {
    id: '1',
    level: 'critical',
    title: 'CPU使用率超过95%',
    description: '服务器CPU使用率持续超过95%，可能导致服务不可用',
    source: '服务器-001',
    time: new Date(Date.now() - 10 * 60 * 1000), // 10分钟前
    status: 'pending',
  },
  {
    id: '2',
    level: 'warning',
    title: '内存使用率超过85%',
    description: '服务器内存使用率超过85%，建议优化内存使用',
    source: '服务器-002',
    time: new Date(Date.now() - 30 * 60 * 1000), // 30分钟前
    status: 'pending',
  },
  {
    id: '3',
    level: 'info',
    title: '磁盘空间不足',
    description: '磁盘空间剩余不足10%，建议清理或扩容',
    source: '存储-001',
    time: new Date(Date.now() - 2 * 60 * 60 * 1000), // 2小时前
    status: 'acknowledged',
  },
  {
    id: '4',
    level: 'critical',
    title: '服务不可用',
    description: 'API服务响应超时，可能已宕机',
    source: '应用-003',
    time: new Date(Date.now() - 5 * 60 * 1000), // 5分钟前
    status: 'pending',
  },
  {
    id: '5',
    level: 'warning',
    title: '网络延迟过高',
    description: '网络延迟超过200ms，可能影响用户体验',
    source: '网络-001',
    time: new Date(Date.now() - 1 * 24 * 60 * 60 * 1000), // 1天前
    status: 'resolved',
  },
]);

// 筛选表单
const showFilter = ref(false);
const filterForm = ref({
  levels: ['critical', 'warning', 'info'] as AlertLevel[],
  statuses: ['pending', 'acknowledged', 'resolved'] as AlertStatus[],
});

// 应用筛选
const filteredAlerts = computed(() => {
  return alerts.value.filter(alert => {
    return (
      filterForm.value.levels.includes(alert.level) &&
      filterForm.value.statuses.includes(alert.status)
    );
  });
});

// 格式化时间
const formatTime = (time: Date) => {
  return dayjs(time).format('MM-DD HH:mm');
};

// 获取告警级别文本
const getLevelText = (level: AlertLevel) => {
  switch (level) {
    case 'critical': return '紧急';
    case 'warning': return '警告';
    case 'info': return '信息';
    default: return level;
  }
};

// 获取告警级别颜色
const getLevelColor = (level: AlertLevel) => {
  switch (level) {
    case 'critical': return 'red';
    case 'warning': return 'orange';
    case 'info': return 'blue';
    default: return 'gray';
  }
};

// 刷新告警
const refreshAlerts = () => {
  Message.success('告警信息已刷新');
};

// 确认告警
const handleAcknowledge = (alert: Alert) => {
  alert.status = 'acknowledged';
  Message.success(`已确认告警: ${alert.title}`);
};

// 解决告警
const handleResolve = (alert: Alert) => {
  alert.status = 'resolved';
  Message.success(`已解决告警: ${alert.title}`);
};

// 应用筛选
const applyFilter = () => {
  showFilter.value = false;
};

// 重置筛选
const resetFilter = () => {
  filterForm.value = {
    levels: ['critical', 'warning', 'info'],
    statuses: ['pending', 'acknowledged', 'resolved'],
  };
};
</script>

<style lang="less" scoped>
.alerts-panel {
  background-color: var(--color-bg-2);
  border-radius: 4px;
  overflow: hidden;
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background-color: var(--color-fill-2);
  border-bottom: 1px solid var(--color-border);
  
  .title {
    display: flex;
    align-items: center;
    font-weight: 500;
    
    .icon {
      margin-right: 8px;
      color: var(--color-text-2);
      font-size: 16px;
    }
  }
}

.alerts-content {
  flex: 1;
  overflow: auto;
  position: relative;
  
  :deep(.arco-table) {
    background-color: transparent;
    
    .arco-table-tr {
      cursor: pointer;
      
      &:hover {
        background-color: var(--color-fill-1);
      }
    }
    
    .arco-table-th {
      background-color: transparent;
      font-weight: 500;
    }
  }
}

.empty {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  text-align: center;
  color: var(--color-text-3);
  
  .arco-icon {
    font-size: 48px;
    margin-bottom: 12px;
  }
}

.filter-form {
  padding: 16px;
  
  .arco-form-item {
    margin-bottom: 20px;
  }
  
  .arco-checkbox-group {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
}
</style>