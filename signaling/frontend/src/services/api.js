import axios from 'axios'

const api = axios.create({
  baseURL: '/',
  withCredentials: true,
  timeout: 10000
})

// WebRTC 信令接口
export default {
  startPull(data) {
    return api.post('/signaling/pull', data)
  },
  
  stopPull(data) {
    return api.post('/signaling/stoppull', data)
  },
  
  startPush(data) {
    return api.post('/signaling/push', data)
  },
  
  stopPush(data) {
    return api.post('/signaling/stoppush', data)
  },

  sendAnswer(data) {
    return api.post('/signaling/sendanswer', data)
  }
}