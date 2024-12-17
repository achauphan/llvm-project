// RUN: %check_clang_tidy %s misc-UnusedParameterCustom %t

// Test function declaration with unused parameter
// ===============================================
void foo(int x) {;}
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Parameter 'x' is unused! [misc-UnusedParameterCustom]
// CHECK-MESSAGES: :[[@LINE-2]]:14: warning: Fixing parameter 'x' in foo [misc-UnusedParameterCustom]
// CHECK-FIXES: {{^}}void foo() {;}{{$}}

static void dummy(int x) {;}
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: Parameter 'x' is unused! [misc-UnusedParameterCustom]
// CHECK-MESSAGES: :[[@LINE-2]]:23: warning: Fixing parameter 'x' in dummy [misc-UnusedParameterCustom]
// CHECK-FIXES: static void dummy()


// Test call sites calling with an unused parameter
// ================================================
static void callSites() {
  dummy(6);
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: Fixing argument index 0 at call site  dummy [misc-UnusedParameterCustom]
// CHECK-FIXES: dummy();
}

// Examples with parameters that are used that
// do not get changed
// ===========================================
int bar(int x) { return x;};


