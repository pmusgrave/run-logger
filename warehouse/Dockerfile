FROM --platform=linux/amd64 node
COPY . .
COPY env .
RUN npm install
CMD ["node", "index.js"]
