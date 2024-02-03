import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '../views/HomeView.vue'
import ManageRadioView from '../views/ManageRadioView.vue'
import SupportView from '../views/SupportView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      component: HomeView
    },
    {
      path: '/manage/radios/:radioId',
      name: 'manageRadio',
      component: ManageRadioView,
      props: true
    },
    {
      path: '/support',
      name: 'support',
      component: SupportView
    }
  ]
})

export default router
