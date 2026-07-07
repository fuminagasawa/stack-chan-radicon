import sys
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import *

from udpsender import gen_cmd_audio, gen_cmd_stream, gen_cmd_servo, gen_cmd_eye_auto, gen_cmd_mouth_open
from udpsender import find_stackchan_ip,send_udp_message


class Window(QWidget):

    def __init__(self):
        super().__init__()

        self.initUI()
    
    def initUI(self):
        

        self.servo_angle_names = ["Horizontal","Vertical"]
        servo_count = len(self.servo_angle_names)

        self.servo_angle_label = [QLabel() for x in range(servo_count)]
        for i,each_servo_label in enumerate(self.servo_angle_label):
            each_servo_label.setText(self.servo_angle_names[i])

        self.servo_angle_sliders = []
        self.servo_angle_sliders.append(QSlider(Qt.Orientation.Horizontal))
        self.servo_angle_sliders.append(QSlider(Qt.Orientation.Vertical))


        for each_servo_slider in self.servo_angle_sliders:
            each_servo_slider.setMaximum( 180)
            each_servo_slider.setMinimum(-180)
            each_servo_slider.setValue(0)
            #each_servo_slider.valueChanged.connect(self.value_change)


        servo_frame  = [QFrame()      for x in range(servo_count)]
        servo_layout = [QHBoxLayout() for x in range(servo_count)]

        vbox = QVBoxLayout()

        for i in range(servo_count):
            servo_layout[i].addWidget(self.servo_angle_label[i])
            servo_layout[i].addWidget(self.servo_angle_sliders[i])
            servo_frame[i].setLayout(servo_layout[i])
            vbox.addWidget(servo_frame[i])  
            
        # サーボモータ反映ボタン
        self.apply_servo_button = QPushButton("Apply Servo Angles")
        self.apply_servo_button.clicked.connect(self.set_servo_value)  
        vbox.addWidget(self.apply_servo_button)

        # 自動顔ON/OFFボタン
        self.eye_auto_checkbox = QCheckBox("Enable Eye Auto Control")
        self.eye_auto_checkbox.stateChanged.connect(self.set_auto_eye)
        vbox.addWidget(self.eye_auto_checkbox)

        # 口の開き具合スライダー    
        self.mouth_open_slider = QSlider(Qt.Orientation.Horizontal)
        self.mouth_open_slider.setMaximum(100)
        self.mouth_open_slider.setMinimum(0)
        self.mouth_open_slider.setValue(0)
        self.mouth_open_slider.valueChanged.connect(self.set_mouth_open)
        mouth_layout = QHBoxLayout()
        mouth_layout.addWidget(QLabel("Mouth Open"))
        mouth_layout.addWidget(self.mouth_open_slider)
        mouth_frame = QFrame()
        mouth_frame.setLayout(mouth_layout)
        vbox.addWidget(mouth_frame)





        self.setLayout(vbox)
        self.setGeometry(300, 300, 450, 300)
        self.show()
    
    def set_servo_value(self):

        get_servo_cmd = gen_cmd_servo( self.servo_angle_sliders[0].value(), self.servo_angle_sliders[1].value())#+"\n"
        print(get_servo_cmd)
        send_udp_message(get_servo_cmd, stackchan_ip, stackchan_port)
        pass

    def set_mouth_open(self, open_value):

        get_mouth_cmd = gen_cmd_servo( 0, open_value) #+"\n"
        print(get_mouth_cmd)
        send_udp_message(get_mouth_cmd, stackchan_ip, stackchan_port)
        pass

    def set_auto_eye(self, enable):

        get_eye_auto_cmd = gen_cmd_eye_auto(enable) #+"\n"
        print(get_eye_auto_cmd)
        send_udp_message(get_eye_auto_cmd, stackchan_ip, stackchan_port)
        pass


    def set_mouth_open(self, open_value):

        get_mouth_cmd = gen_cmd_mouth_open(open_value) #+"\n"
        print(get_mouth_cmd)
        send_udp_message(get_mouth_cmd, stackchan_ip, stackchan_port)
        pass


if __name__ == "__main__":

    #message = r"Hello, UDP!"
    stackchan_mac = "48-27-e2-7a-37-7c"
    stackchan_port = 12345

    print("Finding Stackchan IP...")
    stackchan_ip = find_stackchan_ip(stackchan_mac)
    print(f"Stackchan IP: {stackchan_ip}")



    app = QApplication(sys.argv)
    ex = Window()

    sys.exit(app.exec())