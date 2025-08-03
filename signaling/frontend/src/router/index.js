import { createRouter, createWebHistory } from 'vue-router'
import PushView from '@/views/PushView.vue'
import PullView from '@/views/PullView.vue'

const router = createRouter({
    history: createWebHistory(),
    routes: [
        {
            path: '/xrtcclient/push',
            name: 'push',
            component: PushView,
            props: route => ({
                uid: route.query.uid || '',
                streamName: route.query.streamName || '',
                audio: route.query.audio || '1',
                video: route.query.video || '1'
            })
        },
        {
            path: '/xrtcclient/pull',
            name: 'pull',
            component: PullView,
            props: route => ({
                uid: route.query.uid || '',
                streamName: route.query.streamName || '',
                audio: route.query.audio || '1',
                video: route.query.video || '1'
            })
        },
        {
            path: '/:pathMatch(.*)*',
            redirect: '/xrtcclient/push'
        }
    ]
})

export default router