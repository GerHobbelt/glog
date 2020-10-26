Pod::Spec.new do |spec|
  spec.name = 'glogJM'
  spec.version = '0.3.5-alpha'
  spec.license = { :type => 'Google', :file => 'COPYING' }
  spec.homepage = 'https://github.com/JimiPlatform/glog'
  spec.summary = 'glog for iOS ReactNative'
  spec.authors = 'Jimi,Google'

  spec.source = { :git => 'https://github.com/JimiPlatform/glog.git', :tag => "#{spec.version}" }
  spec.libraries           = "stdc++"
  spec.pod_target_xcconfig = { "USE_HEADERMAP" => "NO",
                               "HEADER_SEARCH_PATHS" => "$(PODS_TARGET_SRCROOT)/src" }

  spec.platform = :ios, "9.0"
  spec.xcconfig = { 'VALID_ARCHS' => 'armv7s armv7 x86_64 arm64 arm64e' }
  spec.ios.vendored_frameworks = 'glog.framework'

end


#校验指令
#pod lib lint glogJM.podspec --verbose --allow-warnings --use-libraries
#发布命令
#pod trunk push glogJM.podspec --verbose --allow-warnings --use-libraries
