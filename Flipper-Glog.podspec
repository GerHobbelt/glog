Pod::Spec.new do |s|
    s.name         = "Flipper-Glog"
    s.version      = "0.3.7"
    s.homepage     = "https://github.com/priteshrnandgaonkar/glog/"
    s.source       = { :git => 'https://github.com/priteshrnandgaonkar/glog.git', :tag => "v0.3.7" }
    s.license      = { :type => 'Google', :file => 'COPYING' }
    s.summary      = 'Google logging module'
    s.authors      = {'Prtesh Nandgaonkar' => 'prit.nandgaonkar@gmail.com'}
    s.cocoapods_version = '>= 1.9'
    s.ios.deployment_target = '9.0'
    s.vendored_frameworks = 'Frameworks/glog.xcframework'
  end
