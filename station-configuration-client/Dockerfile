# https://vuejs.org/v2/cookbook/dockerize-vuejs-app.html

# build stage
FROM node:17 as build-stage
WORKDIR /app
COPY package*.json ./
RUN npm install
COPY . .

RUN npm run build

# production stage
FROM nginx:stable-alpine as production-stage

COPY --from=build-stage /app/dist /usr/share/nginx/html/

COPY default.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]