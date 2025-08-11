<template>
  <div class="desktop-list">
    <a-page-header title="桌面列表" subtitle="管理云桌面">
      <template #extra>
        <a-button type="primary" @click="showCreateModal">
          <template #icon>
            <icon-plus />
          </template>
          添加桌面
        </a-button>
      </template>
    </a-page-header>

    <a-table :columns="columns" :data="desktops" :pagination="pagination" row-key="id" style="margin-top: 20px">
      <template #status="{ record }">
        <a-tag :color="record.status === 'online' ? 'green' : 'red'">
          {{ record.status === 'online' ? '在线' : '离线' }}
        </a-tag>
      </template>

      <template #action="{ record }">
        <a-button type="text" @click="startDesktop(record)">开机</a-button>

        <a-button type="text" :loading="record.connecting" @click="connectDesktop(record)">
          {{ record.connecting ? '连接中...' : '连接' }}
        </a-button>
        <a-button type="text" @click="editDesktop(record)">编辑</a-button>
        <a-button type="text" status="danger" @click="shutdownDesktop(record)">
          关机
        </a-button>
        <a-button type="text" status="danger" @click="deleteDesktop(record)">
          删除
        </a-button>
      </template>
    </a-table>

    <!-- 连接状态模态框 -->
    <a-modal v-model:visible="connectionModalVisible" title="正在连接桌面" :simple="true" :width="400" :footer="false"
      :closable="false" :mask-closable="false">
      <div class="connection-modal">
        <a-spin :loading="connecting" size="large">
          <div class="connection-progress">
            <div class="progress-bar" :style="{ width: progress + '%' }"></div>
          </div>
          <div class="connection-message">
            {{ connectionMessage }}
          </div>
        </a-spin>
      </div>
    </a-modal>

    <!-- 创建桌面模态框 -->
    <a-modal v-model:visible="createModalVisible" title="添加新桌面" :width="600" @ok="handleCreate">
      <a-form :model="newDesktop">
        <a-form-item label="桌面名称">
          <a-input v-model="newDesktop.name" placeholder="请输入桌面名称" />
        </a-form-item>
        <a-form-item label="IP地址">
          <a-input v-model="newDesktop.ip" placeholder="请输入IP地址" />
        </a-form-item>
        <a-form-item label="操作系统">
          <a-select v-model="newDesktop.os" placeholder="请选择操作系统">
            <a-option value="windows">Windows</a-option>
            <a-option value="linux">Linux</a-option>
            <a-option value="macos">macOS</a-option>
          </a-select>
        </a-form-item>
        <a-form-item label="描述">
          <a-textarea v-model="newDesktop.description" placeholder="请输入描述" />
        </a-form-item>
      </a-form>
    </a-modal>
  </div>
</template>

<script lang="ts" setup>
import { ref } from 'vue';
import { useRouter } from 'vue-router';
import { Message } from '@arco-design/web-vue';
import dayjs from 'dayjs';

const router = useRouter();

// 表格列定义
const columns = [
  {
    title: '桌面名称',
    dataIndex: 'name',
  },
  {
    title: 'IP地址',
    dataIndex: 'ip',
  },
  {
    title: '操作系统',
    dataIndex: 'os',
  },
  {
    title: '状态',
    slotName: 'status',
  },
  {
    title: '最后活动',
    dataIndex: 'lastActive',
  },
  {
    title: '操作',
    slotName: 'action',
  },
];

// 桌面数据
const desktops = ref([
  {
    id: '1',
    name: 'windows桌面',
    ip: '123.60.30.56',
    os: 'windows',
    status: 'online',
    lastActive: dayjs().format('YYYY-MM-DD HH:mm'),
    connecting: false,
  },
  {
    id: '2',
    name: 'ubuntu桌面',
    ip: '120.46.144.86',
    os: 'linux',
    status: 'offline',
    lastActive: dayjs().subtract(1, 'day').set('hour', 18).set('minute', 34).format('YYYY-MM-DD HH:mm'),
    connecting: false, // 连接状态
  },
]);

// 分页配置
const pagination = ref({
  current: 1,
  pageSize: 10,
  total: 2,
  showPageSize: true,
  showJumper: true,
  showTotal: true,
});

// 创建桌面模态框
const createModalVisible = ref(false);
const newDesktop = ref({
  name: '',
  ip: '',
  os: '',
  description: '',
});

// 连接状态模态框
const connectionModalVisible = ref(false);
const connecting = ref(false);
const connectionMessage = ref('正在建立安全连接...');
const progress = ref(0);
const currentConnectingDesktop = ref<any>(null);

// 显示创建模态框
const showCreateModal = () => {
  createModalVisible.value = true;
};

// 处理创建桌面
const handleCreate = () => {
  // 创建桌面的逻辑
  Message.success('桌面创建成功');
  createModalVisible.value = false;
};

// 连接桌面（在新标签页打开）
const connectDesktop = (desktop) => {
  // 防止重复点击
  if (desktop.connecting) return;

  // 设置连接状态
  desktop.connecting = true;

  // 处理离线桌面
  if (desktop.status === 'offline') {
    // 显示开机提示
    Message.info(`正在为 ${desktop.name} 开机，请等待...`);

    // 20秒后完成开机并连接
    setTimeout(() => {
      // 完成开机
      desktop.status = 'online';
      desktop.lastActive = dayjs().format('YYYY-MM-DD HH:mm');

      // 显示开机完成提示
      Message.success(`${desktop.name} 已开机`);

      // 显示连接提示
      Message.info(`正在连接到 ${desktop.name}...`);

      // 2秒后打开新标签页
      setTimeout(() => {
        // 生成唯一ID（用于区分多个连接）
        const connectionId = `${desktop.id}-${Date.now()}`;

        // 构建远程桌面URL
        const remoteDesktopUrl = `${window.location.origin}/remote-desktop?connectionId=${connectionId}&name=${encodeURIComponent(desktop.name)}&ip=${desktop.ip}&os=${desktop.os}`;

        // 在新标签页打开
        window.open(remoteDesktopUrl, '_blank');

        // 重置连接状态
        desktop.connecting = false;

        Message.success(`已连接到 ${desktop.name}`);
      }, 2000);
    }, 20000);
  } else {
    // 在线桌面直接连接
    // 显示连接提示
    Message.info(`正在连接到 ${desktop.name}...`);

    // 2秒后打开新标签页
    setTimeout(() => {
      // 生成唯一ID（用于区分多个连接）
      const connectionId = `${desktop.id}-${Date.now()}`;

      // 构建远程桌面URL
      const remoteDesktopUrl = `${window.location.origin}/remote-desktop?connectionId=${connectionId}&name=${encodeURIComponent(desktop.name)}&ip=${desktop.ip}&os=${desktop.os}`;

      // 在新标签页打开
      window.open(remoteDesktopUrl, '_blank');

      // 重置连接状态
      desktop.connecting = false;

      Message.success(`已连接到 ${desktop.name}`);
    }, 2000);
  }
};

// 编辑桌面
const editDesktop = (desktop) => {
  Message.info(`正在编辑桌面: ${desktop.name}`);
  // 编辑桌面的逻辑
};

// 开机桌面
const startDesktop = (desktop) => {
  if (desktop.status === 'offline') {
    // 显示开机提示
    Message.info(`正在为 ${desktop.name} 开机，请等待...`);

    // 20秒后完成开机
    setTimeout(() => {
      // 完成开机
      desktop.status = 'online';
      desktop.lastActive = dayjs().format('YYYY-MM-DD HH:mm');

      // 显示开机完成提示
      Message.success(`${desktop.name} 已开机`);
    }, 20000);
  } else {
    Message.success(`${desktop.name} 已开机`);
  }
};

// 关机桌面
const shutdownDesktop = (desktop) => {
  if (desktop.status === 'offline') {
    Message.success(`${desktop.name} 已关机`);
    return;
  }
  Message.info(`正在关机: ${desktop.name}`);
  setTimeout(() => {
    Message.success(`${desktop.name} 已关机`);
    desktop.status = 'offline';
  }, 5000);
};

// 删除桌面
const deleteDesktop = (desktop) => {
  Message.info(`正在删除桌面: ${desktop.name}`);
  // 删除桌面的逻辑
};
</script>

<style lang="less" scoped>
.desktop-list {
  background-color: var(--color-bg-2);
  padding: 16px 20px;
  border-radius: 4px;
  height: 100%;
  font-size: 26px;
}

:deep(.arco-page-header-title) {
  font-size: 26px;
}

:deep(.arco-page-header-subtitle) {
  font-size: 26px;
}

:deep(.arco-table) {
  font-size: 26px;

  .arco-table-th {
    font-size: 22px;
    font-weight: 600;
  }

  .arco-table-td {
    font-size: 24px;
  }

  .arco-btn {
    font-size: 22px;
  }

  .arco-tag {
    font-size: 22px;
  }
}

:deep(.arco-modal) {
  .arco-modal-title {
    font-size: 22px;
  }

  .arco-form-item-label {
    font-size: 22px;
  }

  .arco-input,
  .arco-select-view,
  .arco-textarea {
    font-size: 22px;
  }

  .arco-btn {
    font-size: 22px;
  }
}

:deep(.arco-pagination) {

  .arco-pagination-item,
  .arco-pagination-jumper {
    font-size: 18px;
  }
}

:deep(.arco-btn) {
  font-size: 20px;
}

/* 连接状态模态框样式 */
  .connection-modal {
    text-align: center;
    padding: 20px;

    .connection-progress {
      height: 10px;
      background-color: var(--color-fill-3);
      border-radius: 5px;
      overflow: hidden;
      margin-bottom: 20px;

      .progress-bar {
        height: 100%;
        background-color: var(--color-primary);
        transition: width 0.2s ease;
      }
    }

    .connection-message {
      font-size: 22px;
      color: var(--color-text-2);
    }
  }
</style>
