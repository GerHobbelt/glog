Pod::Spec.new do |s|
    s.name         = "Flipper-Glog"
    s.version      = "0.5.0.5"
    s.homepage     = "https://github.com/lblasa/glog/"
    s.source       = { :git => 'https://github.com/lblasa/glog.git', :tag => "flipper-glog-v0.5.0.5" }
    s.license      = { :type => 'Google', :file => 'COPYING' }
    s.summary      = 'Google logging module'
    s.authors      = {'Lorenzo Blasa' => 'lorenzo.blasa@gmail.com'}
    s.cocoapods_version = '>= 1.9'
    s.ios.deployment_target = '9.0'
    s.vendored_frameworks = 'Frameworks/glog.xcframework'
  end
