const path = require('path');
const fs = require('fs');
const webpack = require('webpack');
const CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = {
  outputDir: '../static',
  publicPath: '/static/',
  indexPath: 'index.html',
  
  devServer: {
    https: {
      key: fs.readFileSync(path.resolve(__dirname, '../conf/privkey.pem')),
      cert: fs.readFileSync(path.resolve(__dirname, '../conf/fullchain.pem')),
    },
    port: 8081,
    proxy: {
      '^/signaling': {
        target: 'https://localhost:8081',
        secure: false
      }
    }
  },
  
  configureWebpack: {
    plugins: [
      // 复制必要的库文件
      new CopyWebpackPlugin({
        patterns: [
          {
            from: path.resolve(__dirname, 'public/js'),
            to: path.resolve(__dirname, '../static/js')
          }
        ]
      }),
      // 提供全局 jQuery 支持
      new webpack.ProvidePlugin({
        $: 'jquery',
        jQuery: 'jquery'
      })
    ],
    resolve: {
      alias: {
        '@': path.resolve(__dirname, 'src')
      }
    }
  },
  
  chainWebpack: config => {
    config.plugin('html').tap(args => {
      args[0].title = 'WebRTC云桌面';
      return args;
    });
    
    // 排除 adapter 从打包
    config.externals({
      'webrtc-adapter': 'adapter'
    });
  }
}