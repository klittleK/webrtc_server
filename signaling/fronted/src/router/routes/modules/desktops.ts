import { DEFAULT_LAYOUT } from '../base';
import { AppRouteRecordRaw } from '../types';

const DESKTOPS: AppRouteRecordRaw = {
  path: '/desktops',
  name: 'desktops',
  component: DEFAULT_LAYOUT, // 使用布局组件
  meta: {
    locale: 'menu.desktops',
    requiresAuth: true,
    icon: 'icon-robot',
    order: 0,
    hideInMenu: false,
  },
  children: [
    {
      path: '', // 空路径表示默认子路由
      name: 'desktops-main',
      component: () => import('@/views/desktops/index.vue'),
      meta: {
        locale: 'menu.desktops',
        requiresAuth: true,
        roles: ['*'],
        hideInMenu: true, // 在菜单中隐藏此子路由
        activeMenu: 'desktops', // 高亮父级菜单
      },
    },
  ],
};

export default DESKTOPS;